/**
  ******************************************************************************
  * @file    usbd_audio.c
  * @brief   USB Audio Class 2.0 with PCM + DoP support
  *          Experimental DSD512 build
  ******************************************************************************
  */

#include "usbd_audio.h"
#include "audio_buffer.h"
#include "usbd_ctlreq.h"
#include "usbd_audio_if.h"
#include "audio_desc.h"
#include "bus_poll_fpga.h"
#include <string.h>

#ifndef USB_HS_MAX_PACKET_SIZE
#define USB_HS_MAX_PACKET_SIZE 1024U
#endif

#ifndef CS_CLOCK_VALID_CONTROL
#define CS_CLOCK_VALID_CONTROL  0x02U
#endif

#define DOP_DSD64_RATE   176400U
#define DOP_DSD128_RATE  352800U
#define DOP_DSD256_RATE  705600U
#define DOP_DSD512_RATE  1411200U

volatile uint32_t g_dbg_pkt_cnt = 0;
volatile uint32_t g_dbg_pkt_size = 0;
volatile uint32_t g_dbg_ovfl_cnt = 0;
volatile uint32_t g_dbg_ab_size_before = 0;
volatile uint8_t g_uac_valid_bits = 32u;

volatile uint8_t  g_dsd_mode    = DSD_MODE_OFF;
volatile uint32_t g_dsd_clock_hz = 0;
volatile uint8_t  g_dop_mode    = 0u;

#ifdef USE_USBD_COMPOSITE
#error "Composite device is unsupported."
#endif

void fpga_set_stream_en(uint8_t en);
void fpga_set_dsd_mode(uint8_t dsd_on);
void fpga_set_dop_mode(uint8_t dop_on);
void fpga_send_resync(void);
int  SampleRate_Init_Si5340(uint32_t fs);

static uint8_t  USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length);
static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length);
static uint8_t  USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t  USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev);
static uint8_t  USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev);
static uint8_t  USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static void     AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void     AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void     AUDIO_REQ_GetRange(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void    *USBD_AUDIO_GetAudioHeaderDesc(uint8_t *pConfDesc);

static USBD_AUDIO_HandleTypeDef s_haudio;

extern volatile uint8_t  g_audio_run;
volatile uint32_t g_fs_pending  = 0;
volatile uint8_t  g_fs_apply_req = 0;
extern TIM_HandleTypeDef htim2;
volatile uint32_t g_fs_meas_hz  = 192000;
static uint32_t   s_tim2_last_cnt = 0;
volatile uint32_t g_usb_fb_16_16;
volatile int32_t  g_usb_fb_adj = 0;

static uint32_t s_acc = 0;
static uint32_t s_n   = 0;
#define FS_AVG_MS   8u

static uint8_t s_dop_detect_count = 0u;
static uint8_t s_dop_last_marker  = 0u;

/* Fragment support for 708/708 split or any non-8-aligned callbacks */
static uint8_t  s_dop_frag[8];
static uint32_t s_dop_frag_len = 0u;

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

void delay_us(uint16_t us)
{
    uint32_t i = 0;
    us = us * 4;
    while (i <= us) { i++; }
}

void audio_fs_measure_start(void)
{
    __HAL_TIM_DISABLE(&htim2);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    s_tim2_last_cnt = 0;
    s_acc = 0;
    s_n   = 0;
    __HAL_TIM_ENABLE(&htim2);
}

void audio_fs_measure_stop(void)
{
    __HAL_TIM_DISABLE(&htim2);
}

static inline void usb_pack_fb_u32_le(uint32_t fb, uint8_t out4[4])
{
    out4[0] = (uint8_t)(fb & 0xFFu);
    out4[1] = (uint8_t)((fb >>  8) & 0xFFu);
    out4[2] = (uint8_t)((fb >> 16) & 0xFFu);
    out4[3] = (uint8_t)((fb >> 24) & 0xFFu);
}

#ifndef UAC_FB_USE_Q14_18
#define UAC_FB_USE_Q14_18 0
#endif

static inline uint32_t uac_fb_from_fs(uint32_t fs_hz)
{
#if UAC_FB_USE_Q14_18
    return (uint32_t)(((uint64_t)fs_hz << 18) / 8000ull);
#else
    return (uint32_t)(((uint64_t)fs_hz << 16) / 8000ull);
#endif
}

static inline uint8_t is_dop_carrier_rate(uint32_t fs)
{
    return (uint8_t)((fs == DOP_DSD64_RATE)  ||
                     (fs == DOP_DSD128_RATE) ||
                     (fs == DOP_DSD256_RATE) ||
                     (fs == DOP_DSD512_RATE));
}

static inline void dop_detect_reset(void)
{
    s_dop_detect_count = 0u;
    s_dop_last_marker  = 0u;
}

static inline void dop_stream_reset(void)
{
    dop_detect_reset();
    s_dop_frag_len = 0u;
}

static inline uint8_t dop_is_marker(uint8_t b)
{
    return (uint8_t)((b == 0x05u) || (b == 0xFAu));
}

static uint8_t dop_scan_packet(const uint8_t *pkt, uint32_t len)
{
    if ((pkt == NULL) || (len < 8u) || ((len & 7u) != 0u)) {
        dop_detect_reset();
        return 0u;
    }

    {
        const uint32_t frames = len / 8u;

        for (uint32_t i = 0u; i < frames; i++) {
            const uint8_t *f = &pkt[i * 8u];
            uint8_t marker = 0u;
            uint8_t ok = 0u;

            if ((f[0] == 0x00u) && (f[4] == 0x00u) &&
                (f[3] == f[7]) && dop_is_marker(f[3])) {
                marker = f[3];
                ok = 1u;
            }
            else if ((f[2] == f[6]) && dop_is_marker(f[2])) {
                marker = f[2];
                ok = 1u;
            }

            if (!ok) {
                dop_detect_reset();
                return 0u;
            }

            if ((s_dop_detect_count != 0u) && (marker == s_dop_last_marker)) {
                dop_detect_reset();
                return 0u;
            }

            s_dop_last_marker = marker;
            if (s_dop_detect_count < 255u) {
                s_dop_detect_count++;
            }

            if (s_dop_detect_count >= 4u) {
                return 1u;
            }
        }
    }

    return 0u;
}

static uint8_t dop_scan_stream(const uint8_t *pkt, uint32_t len)
{
    uint8_t tmp[8];
    uint32_t use;

    if ((pkt == NULL) || (len == 0u)) {
        dop_stream_reset();
        return 0u;
    }

    if (s_dop_frag_len != 0u) {
        use = 8u - s_dop_frag_len;

        if (use > len) {
            memcpy(&s_dop_frag[s_dop_frag_len], pkt, len);
            s_dop_frag_len += len;
            return 0u;
        }

        memcpy(tmp, s_dop_frag, s_dop_frag_len);
        memcpy(&tmp[s_dop_frag_len], pkt, use);

        s_dop_frag_len = 0u;

        if (dop_scan_packet(tmp, 8u)) {
            return 1u;
        }

        pkt += use;
        len -= use;
    }

    use = (len & ~7u);
    if (use != 0u) {
        if (dop_scan_packet(pkt, use)) {
            return 1u;
        }

        pkt += use;
        len -= use;
    }

    if (len != 0u) {
        memcpy(s_dop_frag, pkt, len);
        s_dop_frag_len = len;
    }

    return 0u;
}

static uint32_t dop_rate_to_dsd_clock(uint32_t dop_rate)
{
    if (dop_rate == DOP_DSD64_RATE)  return DSD64_CLOCK;
    if (dop_rate == DOP_DSD128_RATE) return DSD128_CLOCK;
    if (dop_rate == DOP_DSD256_RATE) return DSD256_CLOCK;
    if (dop_rate == DOP_DSD512_RATE) return DSD512_CLOCK;
    return DSD64_CLOCK;
}

static inline uint32_t dop_rate_to_si5340_freq(uint32_t dop_rate)
{
    return dop_rate / 4u;
}

static uint8_t dop_rate_to_dsd_mode(uint32_t dop_rate)
{
    if (dop_rate == DOP_DSD64_RATE)  return DSD_MODE_DSD64;
    if (dop_rate == DOP_DSD128_RATE) return DSD_MODE_DSD128;
    if (dop_rate == DOP_DSD256_RATE) return DSD_MODE_DSD256;
    if (dop_rate == DOP_DSD512_RATE) return DSD_MODE_DSD512;
    return DSD_MODE_OFF;
}

static void force_pcm_mode_runtime(USBD_AUDIO_HandleTypeDef *haudio)
{
    haudio->stream_type = AUDIO_FORMAT_PCM;
    haudio->dsd_mode    = DSD_MODE_OFF;
    g_dsd_mode          = DSD_MODE_OFF;
    g_dsd_clock_hz      = 0u;
    g_dop_mode          = 0u;
    g_uac_valid_bits    = 32u;
    haudio->bit_depth   = 32u;

    fpga_set_dop_mode(0);
    fpga_set_dsd_mode(0);

    dop_stream_reset();
}

static void enter_dop_mode_runtime(USBD_AUDIO_HandleTypeDef *haudio)
{
    const uint32_t dop_rate = haudio->sam_freq;

    if (!is_dop_carrier_rate(dop_rate)) {
        return;
    }

    haudio->stream_type = AUDIO_FORMAT_PCM;
    haudio->dsd_mode    = dop_rate_to_dsd_mode(dop_rate);
    g_dsd_mode          = haudio->dsd_mode;
    g_dsd_clock_hz      = dop_rate_to_dsd_clock(dop_rate);
    g_dop_mode          = 1u;
    g_uac_valid_bits    = 32u;
    haudio->bit_depth   = 32u;

    g_fs_pending   = dop_rate_to_si5340_freq(dop_rate);
    g_fs_apply_req = 1u;

    AudioBuffer_Reset(AudioBuffer_Instance());
    fpga_set_dop_mode(1);
    fpga_set_dsd_mode(1);
    fpga_send_resync();
}

static void enter_pcm_mode(USBD_HandleTypeDef *pdev)
{
    USBD_AUDIO_HandleTypeDef *haudio = pdev->pClassDataCmsit[pdev->classId];

    haudio->stream_type = AUDIO_FORMAT_PCM;
    haudio->dsd_mode    = DSD_MODE_OFF;
    g_dsd_mode          = DSD_MODE_OFF;
    g_dsd_clock_hz      = 0u;
    g_dop_mode          = 0u;
    g_uac_valid_bits    = 32u;
    haudio->bit_depth   = 32u;

    fpga_set_dop_mode(0);
    fpga_set_dsd_mode(0);
    dop_stream_reset();

    AudioBuffer_Reset(AudioBuffer_Instance());
    bus_poll_init();

    fpga_set_stream_en(0);
    delay_us(20);
    fpga_set_stream_en(1);
    delay_us(20);

    {
        lr_sample_t z = { .l = 0, .r = 0 };
        for (int i = 0; i < 512; i++) {
            (void)audrb_push(z);
        }
    }

    bus_poll_start();
    audio_fs_measure_start();

    AUDIO_AudioCmd(NULL, 0, AUDIO_CMD_PLAY);
}

extern volatile uint32_t g_fs_meas_hz;

static void USBD_AUDIO_UpdateFeedbackValue(USBD_HandleTypeDef *pdev)
{
    USBD_AUDIO_HandleTypeDef *haudio = pdev->pClassDataCmsit[pdev->classId];

    uint32_t fs = haudio->sam_freq;
    if (fs < 8000u || fs > AUDIO_MAX_FREQ) {
        fs = 48000u;
    }

    haudio->feedback_value = uac_fb_from_fs(fs);
    g_usb_fb_16_16         = haudio->feedback_value;
}

static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    UNUSED(cfgidx);

    USBD_AUDIO_HandleTypeDef *haudio = &s_haudio;
    pdev->pClassDataCmsit[pdev->classId] = haudio;
    pdev->pClassData = pdev->pClassDataCmsit[pdev->classId];

    if (pdev->dev_speed == USBD_SPEED_HIGH)
    {
        pdev->ep_out[STREAMING_EP_NUM].bInterval = STREAMING_HS_BINTERVAL;
        pdev->ep_in[FEEDBACK_EP_NUM].bInterval   = FEEDBACK_HS_BINTERVAL;
    }
    else
    {
        return USBD_FAIL;
    }

    USBD_LL_FlushEP(pdev, STREAMING_EP_ADDR);
    USBD_LL_FlushEP(pdev, FEEDBACK_EP_ADDR);

    /* IMPORTANT:
     * OpenEP maxpacket must be per-transaction size = 708 bytes
     * PrepareReceive length will be total microframe size = 1416 bytes
     */
    USBD_LL_OpenEP(pdev, STREAMING_EP_ADDR, USBD_EP_TYPE_ISOC, USB_HS_ISO_OUT_MPS_BYTES);
    USBD_LL_OpenEP(pdev, FEEDBACK_EP_ADDR,  USBD_EP_TYPE_ISOC, FEEDBACK_PACKET_SIZE);
    pdev->ep_out[STREAMING_EP_NUM].is_used = 1U;
    pdev->ep_in[FEEDBACK_EP_NUM].is_used   = 1U;

    haudio->alt_setting      = 0u;
    haudio->stream_type      = AUDIO_FORMAT_PCM;
    haudio->dsd_mode         = DSD_MODE_OFF;
    haudio->active_interface = 0u;
    haudio->state            = AUDIO_STATE_STOPPED;
    haudio->bit_depth        = 32u;
    haudio->sam_freq         = 44100u;
    haudio->feedback_base    = uac_fb_from_fs(haudio->sam_freq);
    haudio->feedback_value   = haudio->feedback_base;
    g_usb_fb_16_16           = haudio->feedback_value;
    g_dop_mode               = 0u;

    {
        USBD_AUDIO_ItfTypeDef *itf = pdev->pUserData[pdev->classId];
        if (itf->Init() != USBD_OK)
        {
            return USBD_FAIL;
        }
    }

    USBD_LL_PrepareReceive(pdev, STREAMING_EP_ADDR,
                           (uint8_t*)AudioBuffer_WrPtr(AudioBuffer_Instance()),
                           USB_RX_MAX_PACKET_SIZE);

    usb_pack_fb_u32_le(haudio->feedback_value, g_usb_fb_buf);
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
    pdev->ep_in[FEEDBACK_EP_NUM].is_used  = 0U;
    pdev->ep_in[FEEDBACK_EP_NUM].bInterval = 0U;

    if (pdev->pClassDataCmsit[pdev->classId] != NULL)
    {
        ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData[pdev->classId])->DeInit();
        pdev->pClassDataCmsit[pdev->classId] = NULL;
        pdev->pClassData = NULL;
    }

    g_dsd_mode    = DSD_MODE_OFF;
    g_dsd_clock_hz = 0;
    g_dop_mode    = 0u;
    dop_stream_reset();

    return USBD_OK;
}

static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio = pdev->pClassDataCmsit[pdev->classId];
    uint16_t len;
    uint8_t *pbuf;
    uint16_t status_info = 0U;
    uint8_t interface_num = LOBYTE(req->wIndex);

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
    case USB_REQ_TYPE_CLASS:
        switch (req->bRequest)
        {
        case AUDIO_REQ_CUR:
            if (req->bmRequest & 0x80) { AUDIO_REQ_GetCurrent(pdev, req); }
            else                       { AUDIO_REQ_SetCurrent(pdev, req); }
            break;

        case AUDIO_REQ_RANGE:
            if (req->bmRequest & 0x80) { AUDIO_REQ_GetRange(pdev, req); }
            else
            {
                USBD_CtlError(pdev, req);
                return USBD_FAIL;
            }
            break;

        default:
            USBD_CtlError(pdev, req);
            return USBD_FAIL;
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
            else { USBD_CtlError(pdev, req); return USBD_FAIL; }
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
                else { USBD_CtlError(pdev, req); return USBD_FAIL; }
            }
            break;

        case USB_REQ_GET_INTERFACE:
            if (pdev->dev_state == USBD_STATE_CONFIGURED)
            {
                USBD_CtlSendData(pdev, (uint8_t *)&haudio->alt_setting, 1U);
            }
            else { USBD_CtlError(pdev, req); return USBD_FAIL; }
            break;

        case USB_REQ_SET_INTERFACE:
            if (pdev->dev_state == USBD_STATE_CONFIGURED)
            {
                uint8_t alt_setting = (uint8_t)(req->wValue);

                if (alt_setting <= USBD_MAX_NUM_INTERFACES)
                {
                    haudio->alt_setting      = alt_setting;
                    haudio->active_interface = interface_num;

                    if (interface_num == AS_INTERFACE_NUM)
                    {
                        g_audio_run = (alt_setting != 0);

                        if (g_audio_run)
                        {
                            enter_pcm_mode(pdev);
                        }
                        else
                        {
                            bus_poll_stop();
                            audio_fs_measure_stop();
                            fpga_set_dop_mode(0);
                            fpga_set_dsd_mode(0);
                            g_dsd_mode = DSD_MODE_OFF;
                            g_dop_mode = 0u;
                            dop_stream_reset();
                            AudioBuffer_Reset(AudioBuffer_Instance());
                            AUDIO_AudioCmd(NULL, 0, AUDIO_CMD_STOP);
                            haudio->state = AUDIO_STATE_STOPPED;
                        }
                    }
                }
                else { USBD_CtlError(pdev, req); return USBD_FAIL; }
            }
            else { USBD_CtlError(pdev, req); return USBD_FAIL; }
            break;

        case USB_REQ_CLEAR_FEATURE:
            break;

        default:
            USBD_CtlError(pdev, req);
            return USBD_FAIL;
        }
        break;

    default:
        USBD_CtlError(pdev, req);
        return USBD_FAIL;
    }

    return USBD_OK;
}

extern const uint16_t USBD_AUDIO_CfgDescSize;

static uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length)
{
    *length = USBD_AUDIO_CfgDescSize;
    return (uint8_t *)USBD_AUDIO_CfgDesc;
}

static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
    USBD_AUDIO_HandleTypeDef *haudio = pdev->pClassDataCmsit[pdev->classId];

    if (haudio == NULL) return USBD_FAIL;

    if (haudio->control.unit == CLOCK_SOURCE_ID)
    {
        if (haudio->control.cmd == CS_SAM_FREQ_CONTROL)
        {
            uint32_t new_freq =
                (uint32_t)haudio->control.data[0]        |
                ((uint32_t)haudio->control.data[1] <<  8) |
                ((uint32_t)haudio->control.data[2] << 16) |
                ((uint32_t)haudio->control.data[3] << 24);

            if (new_freq >= AUDIO_MIN_FREQ && new_freq <= AUDIO_MAX_FREQ)
            {
                haudio->sam_freq = new_freq;

                if (haudio->active_interface == AS_INTERFACE_NUM)
                {
                    force_pcm_mode_runtime(haudio);
                    AudioBuffer_Reset(AudioBuffer_Instance());

                    /* Defer DoP DSD256 and DSD512 carriers until marker lock */
                    if ((new_freq == DOP_DSD256_RATE) || (new_freq == DOP_DSD512_RATE))
                    {
                        g_fs_pending   = 0u;
                        g_fs_apply_req = 0u;
                    }
                    else
                    {
                        g_fs_pending   = new_freq;
                        g_fs_apply_req = 1u;
                    }
                }
                else
                {
                    g_fs_pending   = new_freq;
                    g_fs_apply_req = 1u;
                }

                USBD_AUDIO_UpdateFeedbackValue(pdev);
            }
        }
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
    USBD_AUDIO_HandleTypeDef *haudio = pdev->pClassDataCmsit[pdev->classId];
    if (!haudio || haudio->alt_setting == 0) return USBD_OK;

    {
        uint32_t cnt = __HAL_TIM_GET_COUNTER(&htim2);
        uint32_t d   = cnt - s_tim2_last_cnt;
        s_tim2_last_cnt = cnt;

        if (d >= 20u && d <= 800u) {
            s_acc += d;
            if (++s_n >= FS_AVG_MS) {
                g_fs_meas_hz = (s_acc / s_n) * 1000u;
                s_acc = 0u;
                s_n   = 0u;
            }
        }
    }

    return USBD_OK;
}

static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    USBD_AUDIO_HandleTypeDef *haudio =
        (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

    if (!haudio) return USBD_OK;

    if ((epnum == FEEDBACK_EP_NUM) &&
        (haudio->active_interface == AS_INTERFACE_NUM))
    {
        usb_pack_fb_u32_le((uint32_t)((int32_t)g_usb_fb_16_16 + g_usb_fb_adj), g_usb_fb_buf);
        USBD_LL_Transmit(pdev, FEEDBACK_EP_ADDR, g_usb_fb_buf, FEEDBACK_PACKET_SIZE);
    }

    return USBD_OK;
}

void USBD_AUDIO_Sync(USBD_HandleTypeDef *pdev)
{
    USBD_AUDIO_HandleTypeDef *haudio = pdev->pClassDataCmsit[pdev->classId];
    if (haudio == NULL || haudio->state == AUDIO_STATE_STOPPED) return;

    USBD_AUDIO_UpdateFeedbackValue(pdev);
}

static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    USBD_AUDIO_HandleTypeDef *haudio =
        (USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

    if ((haudio != NULL) &&
        (epnum == FEEDBACK_EP_NUM) &&
        (haudio->active_interface == AS_INTERFACE_NUM))
    {
        usb_pack_fb_u32_le((uint32_t)((int32_t)g_usb_fb_16_16 + g_usb_fb_adj), g_usb_fb_buf);
        USBD_LL_Transmit(pdev, FEEDBACK_EP_ADDR, g_usb_fb_buf, FEEDBACK_PACKET_SIZE);
    }

    return USBD_OK;
}

static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    if (epnum == STREAMING_EP_NUM)
    {
        USBD_LL_PrepareReceive(pdev, STREAMING_EP_ADDR,
                               (uint8_t*)AudioBuffer_WrPtr(AudioBuffer_Instance()),
                               USB_RX_MAX_PACKET_SIZE);
    }
    return USBD_OK;
}

static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    USBD_AUDIO_HandleTypeDef *haudio = pdev->pClassDataCmsit[pdev->classId];
    USBD_AUDIO_ItfTypeDef    *itf    = pdev->pUserData[pdev->classId];

    if (haudio == NULL) return (uint8_t)USBD_FAIL;

    if (epnum == STREAMING_EP_NUM)
    {
        AudioBuffer *ab = AudioBuffer_Instance();
        uint8_t *pkt    = (uint8_t*)AudioBuffer_WrPtr(ab);
        uint32_t packetSize = USBD_LL_GetRxDataSize(pdev, epnum);

        g_dbg_pkt_size = packetSize;
        g_dbg_pkt_cnt++;

        if ((haudio->active_interface == AS_INTERFACE_NUM) &&
            (!g_dop_mode) &&
            is_dop_carrier_rate(haudio->sam_freq))
        {
            if (dop_scan_stream(pkt, packetSize)) {
                enter_dop_mode_runtime(haudio);
            }
        }
        else if (!is_dop_carrier_rate(haudio->sam_freq))
        {
            dop_stream_reset();
        }

        AudioBuffer_Receive(ab, packetSize);

        USBD_LL_PrepareReceive(pdev, STREAMING_EP_ADDR,
                               (uint8_t*)AudioBuffer_WrPtr(AudioBuffer_Instance()),
                               USB_RX_MAX_PACKET_SIZE);

        if (haudio->state == AUDIO_STATE_STOPPED)
        {
            itf->AudioCmd(NULL, 0, AUDIO_CMD_PLAY);
            haudio->state = AUDIO_STATE_PLAYING;
        }
    }

    return USBD_OK;
}

static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio = pdev->pClassDataCmsit[pdev->classId];
    uint16_t tx_len = 0U;

    if (haudio == NULL) return;

    USBD_memset(haudio->control.data, 0, USB_MAX_EP0_SIZE);

    switch (HIBYTE(req->wIndex))
    {
    case CLOCK_SOURCE_ID:
        if (HIBYTE(req->wValue) == CS_SAM_FREQ_CONTROL)
        {
            uint32_t fs = haudio->sam_freq;
            if (fs < AUDIO_MIN_FREQ || fs > AUDIO_MAX_FREQ) fs = 48000u;

            haudio->control.data[0] = (uint8_t)(fs & 0xFFu);
            haudio->control.data[1] = (uint8_t)((fs >>  8) & 0xFFu);
            haudio->control.data[2] = (uint8_t)((fs >> 16) & 0xFFu);
            haudio->control.data[3] = (uint8_t)((fs >> 24) & 0xFFu);
            tx_len = 4U;
        }
        else if (HIBYTE(req->wValue) == CS_CLOCK_VALID_CONTROL)
        {
            haudio->control.data[0] = 1U;
            tx_len = 1U;
        }
        else
        {
            USBD_CtlError(pdev, req);
            return;
        }
        break;

    case FEATURE_UNIT_ID:
        USBD_CtlError(pdev, req);
        return;

    default:
        USBD_CtlError(pdev, req);
        return;
    }

    USBD_CtlSendData(pdev, haudio->control.data, MIN(req->wLength, tx_len));
}

static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio = pdev->pClassDataCmsit[pdev->classId];

    if (haudio == NULL) return;

    if (req->wLength != 0U)
    {
        haudio->control.cmd  = HIBYTE(req->wValue);
        haudio->control.len  = (uint8_t)MIN(req->wLength, USB_MAX_EP0_SIZE);
        haudio->control.unit = HIBYTE(req->wIndex);
        USBD_CtlPrepareRx(pdev, haudio->control.data, haudio->control.len);
    }
}

static void AUDIO_REQ_GetRange(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio = pdev->pClassDataCmsit[pdev->classId];
    uint16_t tx_len = 0U;

    if (haudio == NULL) return;

    USBD_memset(haudio->control.data, 0, USB_MAX_EP0_SIZE);

    switch (HIBYTE(req->wIndex))
    {
    case CLOCK_SOURCE_ID:
        if (HIBYTE(req->wValue) == CS_SAM_FREQ_CONTROL)
        {
            haudio->control.data[0]  = 0x01;
            haudio->control.data[1]  = 0x00;

            haudio->control.data[2]  = (uint8_t)(AUDIO_MIN_FREQ & 0xFFu);
            haudio->control.data[3]  = (uint8_t)((AUDIO_MIN_FREQ >> 8) & 0xFFu);
            haudio->control.data[4]  = (uint8_t)((AUDIO_MIN_FREQ >> 16) & 0xFFu);
            haudio->control.data[5]  = (uint8_t)((AUDIO_MIN_FREQ >> 24) & 0xFFu);

            haudio->control.data[6]  = (uint8_t)(AUDIO_MAX_FREQ & 0xFFu);
            haudio->control.data[7]  = (uint8_t)((AUDIO_MAX_FREQ >> 8) & 0xFFu);
            haudio->control.data[8]  = (uint8_t)((AUDIO_MAX_FREQ >> 16) & 0xFFu);
            haudio->control.data[9]  = (uint8_t)((AUDIO_MAX_FREQ >> 24) & 0xFFu);

            haudio->control.data[10] = (uint8_t)(AUDIO_FREQ_RES & 0xFFu);
            haudio->control.data[11] = (uint8_t)((AUDIO_FREQ_RES >> 8) & 0xFFu);
            haudio->control.data[12] = (uint8_t)((AUDIO_FREQ_RES >> 16) & 0xFFu);
            haudio->control.data[13] = (uint8_t)((AUDIO_FREQ_RES >> 24) & 0xFFu);

            tx_len = 14U;
        }
        else { USBD_CtlError(pdev, req); return; }
        break;

    case FEATURE_UNIT_ID:
        if (HIBYTE(req->wValue) == FU_VOLUME_CONTROL)
        {
            haudio->control.data[0] = 0x01;
            haudio->control.data[1] = 0x00;
            haudio->control.data[2] = (uint8_t)(AUDIO_MIN_VOL & 0xFFu);
            haudio->control.data[3] = (uint8_t)((AUDIO_MIN_VOL >> 8) & 0xFFu);
            haudio->control.data[4] = (uint8_t)(AUDIO_MAX_VOL & 0xFFu);
            haudio->control.data[5] = (uint8_t)((AUDIO_MAX_VOL >> 8) & 0xFFu);
            haudio->control.data[6] = (uint8_t)(AUDIO_VOL_RES & 0xFFu);
            haudio->control.data[7] = (uint8_t)((AUDIO_VOL_RES >> 8) & 0xFFu);
            tx_len = 8U;
        }
        else { USBD_CtlError(pdev, req); return; }
        break;

    default:
        USBD_CtlError(pdev, req);
        return;
    }

    USBD_CtlSendData(pdev, haudio->control.data, MIN(req->wLength, tx_len));
}

#ifndef USE_USBD_COMPOSITE
static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length)
{
    *length = USB_LEN_DEV_QUALIFIER_DESC;
    return (uint8_t*)USBD_AUDIO_DeviceQualifierDesc;
}
#endif

uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev,
                                     USBD_AUDIO_ItfTypeDef *fops)
{
    if (fops == NULL) return (uint8_t)USBD_FAIL;
    pdev->pUserData[pdev->classId] = fops;
    return (uint8_t)USBD_OK;
}

static void *USBD_AUDIO_GetAudioHeaderDesc(uint8_t *pConfDesc)
{
    USBD_ConfigDescTypeDef  *desc  = (USBD_ConfigDescTypeDef  *)(void *)pConfDesc;
    USBD_DescHeaderTypeDef  *pdesc = (USBD_DescHeaderTypeDef  *)(void *)pConfDesc;
    uint8_t *pAudioDesc = NULL;
    uint16_t ptr;

    if (desc->wTotalLength > desc->bLength)
    {
        ptr = desc->bLength;
        while (ptr < desc->wTotalLength)
        {
            pdesc = USBD_GetNextDesc((uint8_t *)pdesc, &ptr);
            if ((pdesc->bDescriptorType  == CS_INTERFACE) &&
                (pdesc->bDescriptorSubType == HEADER))
            {
                pAudioDesc = (uint8_t *)pdesc;
                break;
            }
        }
    }
    return pAudioDesc;
}

void USBD_AUDIO_PacketTask(void)
{
    /* no-op */
}
