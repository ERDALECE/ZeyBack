#include "usbd_audio_if.h"
#include "main.h"

extern USBD_HandleTypeDef hUsbDeviceHS;

static uint8_t AUDIO_Init();
static uint8_t AUDIO_DeInit();
uint8_t AUDIO_AudioCmd(uint8_t* pbuf, uint32_t size, uint8_t cmd);
static uint8_t AUDIO_GetState();



// Inactive byte buffer’a dolduruyoruz; dolunca aktif olanı gönderiyoruz.



USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops =
{
  AUDIO_Init,
  AUDIO_DeInit,
  AUDIO_AudioCmd,
  AUDIO_GetState,
};

static void Set_PLL3_Audio_Clock(uint32_t fs)
{


	  if(fs % 44100 == 0) {
	        // 44.1, 88.2, 176.4 kHz için PLL3 ayarları

		//  set_sr_44_48(0);


	    } else {
	        // 48, 96, 192 kHz için PLL3 ayarları

	   // 	set_sr_44_48(1);

	    }


}

static uint8_t AUDIO_Init()
{


  return USBD_OK;
}

static uint8_t AUDIO_DeInit()
{


  return USBD_OK;
}


uint8_t AUDIO_AudioCmd(uint8_t* pbuf, uint32_t size, uint8_t cmd)
{
	//AudioBuffer* aud_buf = AudioBuffer_Instance();
	USBD_AUDIO_HandleTypeDef* haudio = hUsbDeviceHS.pClassDataCmsit[hUsbDeviceHS.classId];

  switch (cmd)
  {
	case AUDIO_CMD_PLAY:

		//if ((haudio->state == AUDIO_STATE_STOPPED) && (haudio->aud_buf->size > (haudio->aud_buf->capacity * 2 / 4))) {
		   // HAL_SAI_Transmit_DMA(&hsai_BlockA2, (uint8_t*)haudio->aud_buf->mem, haudio->aud_buf->capacity >> 2);
		    haudio->state = AUDIO_STATE_PLAYING;
		//}
		//HAL_SAI_Transmit_DMA(&hsai_BlockA2, (uint8_t*)aud_buf->mem, aud_buf->capacity >> 2);
		  //  usb_audio_burst_ready((uint8_t*)aud_buf->mem, aud_buf->capacity >> 2);
		    //LL_GPIO_SetOutputPin(LED1_GPIO_Port, LED1_Pin);

		//LL_GPIO_SetOutputPin(LED2_GPIO_Port, LED2_Pin);
		break;

	case AUDIO_CMD_FORMAT:

//		if (*pbuf == AUDIO_FORMAT_DSD)
//		{
//			HAL_I2S_DMAStop(&AUDIO_I2S_MSTR_HANDLE);
//			RCC_I2S_SetFreq(haudio->sam_freq >> 2);
//			HAL_I2S_Transmit_DMA(&AUDIO_I2S_SLAVE_HANDLE, (uint16_t*)&aud_buf->mem[aud_buf->capacity], aud_buf->capacity >> 2);
//			HAL_I2S_Transmit_DMA(&AUDIO_I2S_MSTR_HANDLE, (uint16_t*)aud_buf->mem, aud_buf->capacity >> 2);
//			LL_GPIO_SetOutputPin(DSDOE_GPIO_Port, DSDOE_Pin);
//		}
		break;

	case AUDIO_CMD_STOP:
		haudio->state = AUDIO_STATE_STOPPED;

		LL_GPIO_ResetOutputPin(LED1_GPIO_Port, LED1_Pin);
		LL_GPIO_ResetOutputPin(LED2_GPIO_Port, LED2_Pin);
		LL_GPIO_ResetOutputPin(LED3_GPIO_Port, LED3_Pin);
		LL_GPIO_ResetOutputPin(LED4_GPIO_Port, LED4_Pin);
		//LL_GPIO_ResetOutputPin(DSDOE_GPIO_Port, DSDOE_Pin);
		break;

	case AUDIO_CMD_FREQ:

		Set_PLL3_Audio_Clock(haudio->sam_freq);
		break;

	case AUDIO_CMD_MUTE:

		break;

	case AUDIO_CMD_VOLUME:

		break;

	default:
		break;
  }

  return USBD_OK;
}

static uint8_t AUDIO_GetState()
{
	return USBD_OK;
}
