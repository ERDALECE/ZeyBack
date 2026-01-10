#include "usbd_audio.h"
#include "audio_buffer.h"
#include "usbd_ctlreq.h"
#include "usbd_audio_if.h"
#include "audio_desc.h"
#include "bus_dma_tim1.h"
#include "usb_iso_raw_fifo.h"

volatile uint8_t g_uac_valid_bits = 32u; // 24 or 32

#ifdef USE_USBD_COMPOSITE
#error "Composite device is unsupported."
#endif

void Audio_SetSampleRate(uint32_t fs);
void OnUsbIsoOut(uint8_t *usb_buf, uint32_t packetSize);
void OnUsbIsoOutFast(uint8_t *usb_buf, uint32_t packetSize);
void fpga_set_stream_en(uint8_t en);


static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length);
static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length);
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_GetRange(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void *USBD_AUDIO_GetAudioHeaderDesc(uint8_t *pConfDesc);

static USBD_AUDIO_HandleTypeDef s_haudio;

extern volatile uint8_t  g_audio_run;
extern volatile uint32_t g_audio_ticks;
volatile uint32_t g_fs_pending = 0;
volatile uint8_t  g_fs_apply_req = 0;

USBD_ClassTypeDef USBD_AUDIO =
{
  USBD_AUDIO_Init,
  USBD_AUDIO_DeInit,
  USBD_AUDIO_Setup,
  USBD_AUDIO_EP0_TxReady,
  USBD_AUDIO_EP0_RxReady,
  USBD_AUDIO_DataIn,
  USBD_AUDIO_DataOut,
  USBD_AUDIO_SOF,
  USBD_AUDIO_IsoINIncomplete,
  USBD_AUDIO_IsoOutIncomplete,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetDeviceQualifierDesc,
};

__attribute__((section(".RAM_D2"), aligned(32))) static uint8_t g_usb_fb_buf[4];


static inline void usb_pack_fb_16_16_4B(uint32_t fb_q16_16, uint8_t out4[4])
{
  out4[0] = (uint8_t)(fb_q16_16 & 0xFFu);
  out4[1] = (uint8_t)((fb_q16_16 >> 8) & 0xFFu);
  out4[2] = (uint8_t)((fb_q16_16 >> 16) & 0xFFu);
  out4[3] = (uint8_t)((fb_q16_16 >> 24) & 0xFFu);
}

static inline uint32_t uac_fb_q16_16(uint32_t fs_hz)
{
  // feedback = samples per microframe (125us) in Q16.16
  // microframes per second = 8000
  return (uint32_t)(((uint64_t)fs_hz << 16) / 8000ull);
}

static inline uint32_t uac_fb_q10_14(uint32_t fs_hz)
{
    return (uint32_t)(((uint64_t)fs_hz << 14) / 8000u);
}
//static uint8_t USBD_AUDIO_GetStreamType(USBD_HandleTypeDef* pdev)
//{
//	uint32_t* buf = (uint32_t*)AudioBuffer_WrPtr();
//	const uint8_t marker_table[] = { 0x05, 0xfa };
//	uint8_t idx = (*buf >> 24) == 0xfa;
//
//	for (uint32_t i = 0; i < AUDIO_DOP_DETECT_COUNT; i += 2)
//	{
//		if (buf[i] >> 24 != marker_table[idx] || buf[i + 1] >> 24 != marker_table[idx])
//		{
//			return AUDIO_FORMAT_PCM;
//		}
//
//		idx ^= 1;
//	}
//
//	return AUDIO_FORMAT_DSD;
//}

static void USBD_AUDIO_UpdateFeedbackValue(USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_HandleTypeDef* haudio = pdev->pClassDataCmsit[pdev->classId];

  // Hedef: ring buffer doluluğunu sabit tut (drift'i kapatır -> klik gider).
  // Not: Pump tarafında LO=2048, HI=4096 kullanıyorsun. Ortası iyi bir hedef: 3072.
  const int32_t target = 3072;
  int32_t fill = (int32_t)audrb_count();
  int32_t err  = fill - target;

  // Basit P kontrol (çok küçük): Q16.16 domainine çevir.
  // err>0 => buffer dolu => host'u yavaşlat => feedback'i azalt.
  // err<0 => buffer boş => host'u hızlandır => feedback'i artır.
  int32_t adj = (err * (int32_t)(1 << 16)) / 4096; // Kp = 1/4096

  int64_t fb = (int64_t)haudio->feedback_base - (int64_t)adj;

  // Windows'ın ignore etmemesi için clamp (±1 sample/microframe)
  int64_t clamp = (1ll << 16);
  int64_t base  = (int64_t)haudio->feedback_base;
  if (fb < base - clamp) fb = base - clamp;
  if (fb > base + clamp) fb = base + clamp;

  haudio->feedback_value = (uint32_t)fb;
}

static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  UNUSED(cfgidx);

  USBD_AUDIO_HandleTypeDef* haudio = &s_haudio;
  //haudio->aud_buf = AudioBuffer_Instance();

  pdev->pClassDataCmsit[pdev->classId] = haudio;
  pdev->pClassData = pdev->pClassDataCmsit[pdev->classId];

  if (pdev->dev_speed == USBD_SPEED_HIGH)
  {
    pdev->ep_out[STREAMING_EP_NUM].bInterval = STREAMING_HS_BINTERVAL;
    pdev->ep_in[FEEDBACK_EP_NUM].bInterval = 	FEEDBACK_HS_BINTERVAL;
  }
  else
  {
  	return USBD_FAIL;
  }

  USBD_LL_FlushEP(pdev, STREAMING_EP_ADDR);
  USBD_LL_FlushEP(pdev, FEEDBACK_EP_ADDR);
  USBD_LL_OpenEP(pdev, STREAMING_EP_ADDR, USBD_EP_TYPE_ISOC, USB_HS_MAX_PACKET_SIZE);
  USBD_LL_OpenEP(pdev, FEEDBACK_EP_ADDR, USBD_EP_TYPE_ISOC, FEEDBACK_PACKET_SIZE);
  pdev->ep_out[STREAMING_EP_NUM].is_used = 1U;
  pdev->ep_in[FEEDBACK_EP_NUM].is_used = 1U;

  haudio->alt_setting = 0;
  haudio->stream_type = AUDIO_FORMAT_PCM;


  /* Initialize the Audio output Hardware layer */
  USBD_AUDIO_ItfTypeDef* itf = pdev->pUserData[pdev->classId];
  if (itf->Init() != USBD_OK)
  {
    return USBD_FAIL;
  }

  /* Prepare Out endpoint to receive 1st packet */
  USBD_LL_PrepareReceive(pdev, STREAMING_EP_ADDR, usb_rx_buf, USB_HS_MAX_PACKET_SIZE);
 // USBD_LL_Transmit(pdev, FEEDBACK_EP_ADDR, (uint8_t*)&haudio->feedback_value, FEEDBACK_PACKET_SIZE);
  usb_pack_fb_16_16_4B(haudio->feedback_value, g_usb_fb_buf);
    USBD_LL_Transmit(pdev, FEEDBACK_EP_ADDR, g_usb_fb_buf, FEEDBACK_PACKET_SIZE);

  return USBD_OK;
}

static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  UNUSED(cfgidx);

  USBD_LL_CloseEP(pdev, STREAMING_EP_ADDR);
  USBD_LL_CloseEP(pdev, FEEDBACK_EP_ADDR);
  pdev->ep_out[STREAMING_EP_NUM].is_used = 0U;
  pdev->ep_out[STREAMING_EP_NUM].bInterval = 0U;
  pdev->ep_out[FEEDBACK_EP_NUM].is_used = 0U;
	pdev->ep_out[FEEDBACK_EP_NUM].bInterval = 0U;

  if (pdev->pClassDataCmsit[pdev->classId] != NULL)
  {
    ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData[pdev->classId])->DeInit();
    pdev->pClassDataCmsit[pdev->classId] = NULL;
    pdev->pClassData = NULL;
  }

  return USBD_OK;
}

static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef* haudio = pdev->pClassDataCmsit[pdev->classId];
  uint16_t len;
  uint8_t *pbuf;
  uint16_t status_info = 0U;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    case USB_REQ_TYPE_CLASS:
    	switch (req->bRequest)
    	{
				case AUDIO_REQ_CUR:
					if (req->bmRequest & 0x80)
					{
						AUDIO_REQ_GetCurrent(pdev, req);
					}
					else
					{
						AUDIO_REQ_SetCurrent(pdev, req);
					}
					break;

				case AUDIO_REQ_RANGE:
					if (req->bmRequest & 0x80)
					{
						AUDIO_REQ_GetRange(pdev, req);
					}
					else
					{
						goto ret_err;
					}
					break;

				default:
					goto ret_err;
					break;
    	}
      break;

    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest)
      {
        case USB_REQ_GET_STATUS:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
          }
          else
          {
          	goto ret_err;
          }
          break;

        case USB_REQ_GET_DESCRIPTOR:
          if (HIBYTE(req->wValue) == CS_DEVICE)
          {
          	pbuf = (uint8_t *)USBD_AUDIO_GetAudioHeaderDesc(pdev->pConfDesc);
            if (pbuf != NULL)
            {
              len = MIN(USB_AUDIO_DESC_SIZE, req->wLength);
              USBD_CtlSendData(pdev, pbuf, len);
            }
            else
            {
            	goto ret_err;
            }
          }
          break;

        case USB_REQ_GET_INTERFACE:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            USBD_CtlSendData(pdev, (uint8_t *)&haudio->alt_setting, 1U);
          }
          else
          {
          	goto ret_err;
          }
          break;

        case USB_REQ_SET_INTERFACE:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            if ((uint8_t)(req->wValue) <= USBD_MAX_NUM_INTERFACES)
            {
              haudio->alt_setting = (uint8_t)(req->wValue);
              g_uac_valid_bits = 32u;
              // Always use 32-bit container on the wire (4 bytes/sample)
              haudio->bit_depth = 32u;

              g_audio_run = (haudio->alt_setting != 0);
              fpga_set_stream_en(g_audio_run);      // PD4 anında FPGA’ya gider

              if (g_audio_run) {
                  // SR'i biliyorsan burada set et (yoksa son set edilen kalır)
                 //  bus_dma_tim1_set_fs(g_fs_pending_or_current);
                  bus_dma_tim1_start();
              } else {
                  bus_dma_tim1_stop();
              }

              if (haudio->alt_setting == 0) {
                              g_uac_valid_bits = 32u;
                              // INTERFACE 0: PLAY DURDURULACAK!

                              AUDIO_AudioCmd(NULL, 0, AUDIO_CMD_STOP);
                          } else if (haudio->alt_setting == 1) {

                              // INTERFACE 1/2: PLAY BAŞLATILACAK!
                              AUDIO_AudioCmd(NULL, 0, AUDIO_CMD_PLAY);
                          }
            }
            else
            {
            	goto ret_err;
            }
          }
          else
          {
          	goto ret_err;
          }
          break;

        case USB_REQ_CLEAR_FEATURE:
          break;

        default:
          goto ret_err;
          break;
      }
      break;

    default:
    	goto ret_err;
      break;
  }

  return USBD_OK;

ret_err:
	USBD_CtlError(pdev, req);
  return USBD_FAIL;
}

#ifndef USE_USBD_COMPOSITE

static uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length)
{
	*length = USB_AUDIO_CONFIG_DESC_SIZE;
  return (uint8_t*)USBD_AUDIO_CfgDesc;
}
#endif /* USE_USBD_COMPOSITE  */

static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_AUDIO_HandleTypeDef* haudio = pdev->pClassDataCmsit[pdev->classId];

  if (epnum == FEEDBACK_EP_NUM)
  {
    // feedback'i ring doluluğuna göre güncelle
    USBD_AUDIO_UpdateFeedbackValue(pdev);

    usb_pack_fb_16_16_4B(haudio->feedback_value, g_usb_fb_buf);
    USBD_LL_Transmit(pdev, FEEDBACK_EP_ADDR, g_usb_fb_buf, FEEDBACK_PACKET_SIZE);
  }

  return (uint8_t)USBD_OK;
}


static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
	USBD_AUDIO_HandleTypeDef* haudio = pdev->pClassDataCmsit[pdev->classId];
	USBD_AUDIO_ItfTypeDef* itf = pdev->pUserData[pdev->classId];

	switch (haudio->control.unit)
	{
	case CLOCK_SOURCE_ID:
		if (haudio->control.cmd == CS_SAM_FREQ_CONTROL)
		{


						uint32_t fs =
						  (uint32_t)haudio->control.data[0]       |
						 ((uint32_t)haudio->control.data[1] << 8) |
						 ((uint32_t)haudio->control.data[2] << 16)|
						 ((uint32_t)haudio->control.data[3] << 24);

						haudio->sam_freq = fs;

						haudio->feedback_base  = uac_fb_q16_16(haudio->sam_freq);
						haudio->feedback_value = haudio->feedback_base;

						/* Stream açıksa feedback’i hemen “kick” et (ilk IN için) */
						usb_pack_fb_16_16_4B(haudio->feedback_value, g_usb_fb_buf);
						USBD_LL_Transmit(pdev, FEEDBACK_EP_ADDR, g_usb_fb_buf, 4);
						itf->AudioCmd(haudio->control.data, haudio->control.len, AUDIO_CMD_FREQ);
						g_fs_pending   = fs;
						g_fs_apply_req = 1;


		}
		else
		{
			return USBD_FAIL;
		}
		break;

	case FEATURE_UNIT_ID:
		switch (haudio->control.cmd)
		{
		case FU_MUTE_CONTROL:
			itf->AudioCmd(haudio->control.data, haudio->control.len, AUDIO_CMD_MUTE);
			break;

		case FU_VOLUME_CONTROL:
			itf->AudioCmd(haudio->control.data, haudio->control.len, AUDIO_CMD_VOLUME);
			break;

		default:
			return USBD_FAIL;
			break;
		}
		break;

	default:
		return USBD_FAIL;
		break;
	}

  return USBD_OK;
}

static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev)
{
	UNUSED(pdev);
  return USBD_OK;
}

static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev)
{
	UNUSED(pdev);
  return USBD_OK;
}

void USBD_AUDIO_Sync(USBD_HandleTypeDef *pdev)
{
	// This function is called from timer ISR. The timer is set to count LRCK
	// and generates events every AUDIO_SYNC_CLK_DIV LRCK cycles.
	// AUDIO_SYNC_CLK_DIV must be set to the same as Counter Period for that timer in CubeMX

  USBD_AUDIO_HandleTypeDef* haudio = pdev->pClassDataCmsit[pdev->classId];
  //USBD_AUDIO_ItfTypeDef* itf = pdev->pUserData[pdev->classId];

  if (haudio->state == AUDIO_STATE_STOPPED)
  {
  	return;
  }


  USBD_AUDIO_UpdateFeedbackValue(pdev);

  /*
  if ((haudio->aud_buf->state == AB_UDFL) && (haudio->state == AUDIO_STATE_PLAYING))
	{
		itf->AudioCmd(NULL, 0, AUDIO_CMD_STOP);
		//haudio->stream_type = AUDIO_FORMAT_PCM;
		haudio->state = AUDIO_STATE_STOPPED;
	}
*/

}

static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
	USBD_AUDIO_HandleTypeDef* haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

	if (epnum == FEEDBACK_EP_NUM)
	{
		USBD_LL_Transmit(pdev, FEEDBACK_EP_ADDR, (uint8_t*)&haudio->feedback_value, FEEDBACK_PACKET_SIZE);
	}

  return USBD_OK;
}

static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
	//USBD_AUDIO_HandleTypeDef* haudio = pdev->pClassDataCmsit[pdev->classId];

	if (epnum == STREAMING_EP_NUM)
	{
		//uint32_t packetSize = USBD_LL_GetRxDataSize(pdev, epnum);
		//USBD_LL_PrepareReceive(pdev, STREAMING_EP_ADDR, usb_rx_buf, packetSize);
		USBD_LL_PrepareReceive(pdev, STREAMING_EP_ADDR, usb_rx_buf, USB_HS_MAX_PACKET_SIZE);
	}

  return USBD_OK;
}

static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
	USBD_AUDIO_HandleTypeDef* haudio = pdev->pClassDataCmsit[pdev->classId];
	    USBD_AUDIO_ItfTypeDef* itf = pdev->pUserData[pdev->classId];


	    if (haudio == NULL)
	        return (uint8_t)USBD_FAIL;

	    if (epnum == STREAMING_EP_NUM)
	    {

	    	uint32_t packetSize = USBD_LL_GetRxDataSize(pdev, epnum);
	    	OnUsbIsoOutFast(usb_rx_buf, packetSize);

	    	//OnUsbIsoOut(usb_rx_buf, packetSize);              // <<< BUNU AÇ

	    	USBD_LL_PrepareReceive(pdev, STREAMING_EP_ADDR,
	    	                       usb_rx_buf, USB_HS_MAX_PACKET_SIZE);  // <<< packetSize değil!


	        if ((haudio->state == AUDIO_STATE_STOPPED) ) {

	            itf->AudioCmd(NULL, 0, AUDIO_CMD_PLAY);
	            haudio->state = AUDIO_STATE_PLAYING;
	        }

  }

  return USBD_OK;
}

static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef* haudio = pdev->pClassDataCmsit[pdev->classId];

  if (haudio == NULL)
  {
    return;
  }

  USBD_memset(haudio->control.data, 0, USB_MAX_EP0_SIZE);

  switch (HIBYTE(req->wIndex))
  {
  case FEATURE_UNIT_ID:
  	break;

  case CLOCK_SOURCE_ID:
  	break;

  default:
  	USBD_CtlError(pdev, req);
  	break;
  }

  USBD_CtlSendData(pdev, haudio->control.data, MIN(req->wLength, USB_MAX_EP0_SIZE));
}

static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef* haudio = pdev->pClassDataCmsit[pdev->classId];

  if (haudio == NULL)
  {
    return;
  }

  if (req->wLength != 0U)
  {
  	haudio->control.cmd = HIBYTE(req->wValue);
  	haudio->control.len = (uint8_t)MIN(req->wLength, USB_MAX_EP0_SIZE);
  	haudio->control.unit = HIBYTE(req->wIndex);

  	USBD_CtlPrepareRx(pdev, haudio->control.data, haudio->control.len);
  }
}

static void AUDIO_REQ_GetRange(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	USBD_AUDIO_HandleTypeDef* haudio = pdev->pClassDataCmsit[pdev->classId];

	if (haudio == NULL)
	{
		return;
	}

	USBD_memset(haudio->control.data, 0, USB_MAX_EP0_SIZE);

	uint8_t* pbuf = haudio->control.data;

	switch (HIBYTE(req->wIndex))
	{
	case CLOCK_SOURCE_ID:
		if (HIBYTE(req->wValue) == CS_SAM_FREQ_CONTROL)
		{
			PACK_DATA(pbuf, uint16_t, 1U);
			PACK_DATA(pbuf, uint32_t, AUDIO_MIN_FREQ);
			PACK_DATA(pbuf, uint32_t, AUDIO_MAX_FREQ);
			PACK_DATA(pbuf, uint32_t, AUDIO_FREQ_RES);
		}
		else
		{
			USBD_CtlError(pdev, req);
			return;
		}
		break;

	case FEATURE_UNIT_ID:
		if (HIBYTE(req->wValue) == FU_VOLUME_CONTROL)
		{
			PACK_DATA(pbuf, uint16_t, 1U);
			PACK_DATA(pbuf, uint16_t, AUDIO_MIN_VOL);
			PACK_DATA(pbuf, uint16_t, AUDIO_MAX_VOL);
			PACK_DATA(pbuf, uint16_t, AUDIO_VOL_RES);
		}
		else
		{
			USBD_CtlError(pdev, req);
			return;
		}
		break;

	default:
		USBD_CtlError(pdev, req);
		return;
		break;
	}

	USBD_CtlSendData(pdev, haudio->control.data, MIN(req->wLength, USB_MAX_EP0_SIZE));
}

#ifndef USE_USBD_COMPOSITE

static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length)
{
  *length = USB_LEN_DEV_QUALIFIER_DESC;

  return (uint8_t*)USBD_AUDIO_DeviceQualifierDesc;
}

#endif /* USE_USBD_COMPOSITE  */

uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev,
                                     USBD_AUDIO_ItfTypeDef *fops)
{
  if (fops == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  pdev->pUserData[pdev->classId] = fops;

  return (uint8_t)USBD_OK;
}

static void *USBD_AUDIO_GetAudioHeaderDesc(uint8_t *pConfDesc)
{
  USBD_ConfigDescTypeDef *desc = (USBD_ConfigDescTypeDef *)(void *)pConfDesc;
  USBD_DescHeaderTypeDef *pdesc = (USBD_DescHeaderTypeDef *)(void *)pConfDesc;
  uint8_t *pAudioDesc =  NULL;
  uint16_t ptr;

  if (desc->wTotalLength > desc->bLength)
  {
    ptr = desc->bLength;

    while (ptr < desc->wTotalLength)
    {
      pdesc = USBD_GetNextDesc((uint8_t *)pdesc, &ptr);
      if ((pdesc->bDescriptorType == CS_INTERFACE) &&
          (pdesc->bDescriptorSubType == HEADER))
      {
        pAudioDesc = (uint8_t *)pdesc;
        break;
      }
    }
  }
  return pAudioDesc;
}

