/**
  ******************************************************************************
  * @file    usbd_audio.h
  * @brief   USB Audio Class 2.0 with PCM + DoP support only
  *          Experimental DSD512 build
  ******************************************************************************
  */

#ifndef __USB_AUDIO_H
#define __USB_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"

/*============================================================================
 * DSD / DoP Configuration
 *============================================================================*/
#define DSD64_RATE                  352800U
#define DSD128_RATE                 705600U
#define DSD256_RATE                 1411200U
#define DSD512_RATE                 2822400U

#define DSD64_CLOCK                 2822400U
#define DSD128_CLOCK                5644800U
#define DSD256_CLOCK                11289600U
#define DSD512_CLOCK                22579200U

/*============================================================================
 * PCM / UAC2 Configuration
 *============================================================================*/
#define AUDIO_DOP_DETECT_COUNT      16U

#define AUDIO_48K_FEEDBACK_VALUE    0x2EE000
#define AUDIO_44K1_FEEDBACK_VALUE   0x2C6A7F
#define AUDIO_FEEDBACK_GAIN         8

#define AUDIO_MIN_FREQ              44100U
#define AUDIO_MAX_FREQ              1411200U
#define AUDIO_FREQ_RES              1U

#define AUDIO_MIN_VOL               0U
#define AUDIO_MAX_VOL               100U
#define AUDIO_VOL_RES               1U

/*
 * HS high-bandwidth Iso OUT settings for DSD512 DoP
 *
 * Descriptor encoding:
 *   bits[10:0] = bytes per transaction = 708
 *   bits[12:11] = additional transactions = 1  => total 2 transactions
 *
 * Result:
 *   708 x 2 = 1416 bytes/microframe
 */
#define USB_HS_ISO_OUT_MPS_BYTES    708U
#define USB_HS_ISO_OUT_MULT         2U
#define USB_AUDIO_OUT_EP_WMAXPACKET_HS \
    ((uint16_t)(USB_HS_ISO_OUT_MPS_BYTES | ((USB_HS_ISO_OUT_MULT - 1U) << 11)))

/* Total RX transfer length for one microframe */
#define USB_RX_MAX_PACKET_SIZE      (USB_HS_ISO_OUT_MPS_BYTES * USB_HS_ISO_OUT_MULT)

#define FEEDBACK_HS_BINTERVAL       4U
#define STREAMING_HS_BINTERVAL      1U

#define AUDIO_WTOTALLENGTH          60U
#define USB_AUDIO_CONFIG_DESC_SIZE  148U
#define USB_AUDIO_DESC_SIZE         0x09U

#define FORMAT_TYPE_I               0x01U

#define AUDIO_REQ_CUR               0x01U
#define AUDIO_REQ_RANGE             0x02U

#define FEEDBACK_FORMAT_16_16       1U
#define FEEDBACK_PACKET_SIZE        4U

#define AUDIO_BUFFER_PACKET_NUM     60U
#define AUDIO_BUF_SIZE              (AUDIO_BUFFER_PACKET_NUM * (AUDIO_MAX_FREQ / 1000U))

#define AUDIO_FUNCTION               AUDIO
#define FUNCTION_SUBCLASS_UNDEFINED  0x00
#define FUNCTION_PROTOCOL_UNDEFINED  0x00
#define AF_VERSION_02_00             IP_VERSION_02_00
#define AUDIO                        0x01
#define INTERFACE_SUBCLASS_UNDEFINED 0x00
#define AUDIOCONTROL                 0x01
#define AUDIOSTREAMING               0x02
#define INTERFACE_PROTOCOL_UNDEFINED 0x00
#define IP_VERSION_02_00             0x20

#define CS_UNDEFINED                 0x20
#define CS_DEVICE                    0x21
#define CS_CONFIGURATION             0x22
#define CS_STRING                    0x23
#define CS_INTERFACE                 0x24
#define CS_ENDPOINT                  0x25

#define CS_SAM_FREQ_CONTROL          0x01
#define FU_MUTE_CONTROL              0x01
#define FU_VOLUME_CONTROL            0x02

#define AC_DESCRIPTOR_UNDEFINED      0x00
#define HEADER                       0x01
#define INPUT_TERMINAL               0x02
#define OUTPUT_TERMINAL              0x03
#define CLOCK_SOURCE                 0x0A
#define FEATURE_UNIT                 0x06

#define AS_DESCRIPTOR_UNDEFINED      0x00
#define AS_GENERAL                   0x01
#define FORMAT_TYPE                  0x02

#define AC_INTERFACE_NUM             0x00
#define AS_INTERFACE_NUM             0x01

#define CLOCK_SOURCE_ID              0x04

#define INPUT_TERMINAL_ID            0x01
#define INPUT_TERMINAL_TYPE          0x0101
#define OUTPUT_TERMINAL_ID           0x03
#define OUTPUT_TERMINAL_TYPE         0x0301
#define FEATURE_UNIT_ID              0x02

#define STREAMING_EP_ADDR            0x01
#define STREAMING_EP_ATTRIB          0x05
#define FEEDBACK_EP_ADDR             (STREAMING_EP_ADDR | 0x80)
#define FEEDBACK_EP_ATTRIB           0x11
#define STREAMING_EP_NUM             (STREAMING_EP_ADDR & 0x0F)
#define FEEDBACK_EP_NUM              (FEEDBACK_EP_ADDR & 0x0F)

#define EP_GENERAL                   0x01

#define PACK_DATA(_ptr, _type, _value) do{ \
    *(_type*)_ptr = (_value); \
    _ptr += sizeof(_type); } while(0)

typedef enum
{
    AUDIO_CMD_PLAY,
    AUDIO_CMD_FORMAT,
    AUDIO_CMD_STOP,
    AUDIO_CMD_FREQ,
    AUDIO_CMD_MUTE,
    AUDIO_CMD_VOLUME,
} AUDIO_CommandTypeDef;

typedef enum
{
    AUDIO_FORMAT_PCM = 0,
} AUDIO_FormatTypeDef;

typedef enum
{
    AUDIO_STATE_STOPPED,
    AUDIO_STATE_PLAYING,
} AUDIO_StateTypeDef;

typedef enum
{
    DSD_MODE_OFF = 0,
    DSD_MODE_DSD64 = 1,
    DSD_MODE_DSD128 = 2,
    DSD_MODE_DSD256 = 3,
    DSD_MODE_DSD512 = 4
} DSD_ModeTypeDef;

typedef struct
{
    uint8_t data[256];
    uint8_t cmd;
    uint8_t len;
    uint8_t unit;
} USBD_AUDIO_ControlTypeDef;

typedef struct
{
    USBD_AUDIO_ControlTypeDef control;
    uint32_t alt_setting;
    uint32_t sam_freq;
    uint32_t feedback_base;
    uint32_t feedback_value;
    uint8_t bit_depth;
    uint8_t stream_type;
    uint8_t state;
    uint8_t dsd_mode;
    uint8_t active_interface;
} USBD_AUDIO_HandleTypeDef;

typedef struct
{
    uint8_t (*Init)();
    uint8_t (*DeInit)();
    uint8_t (*AudioCmd)(uint8_t* pbuf, uint32_t size, uint8_t cmd);
    uint8_t (*GetState)();
} USBD_AUDIO_ItfTypeDef;

extern USBD_ClassTypeDef USBD_AUDIO;
#define USBD_AUDIO_CLASS &USBD_AUDIO

extern volatile uint8_t g_uac_valid_bits;
extern volatile uint8_t g_dsd_mode;
extern volatile uint32_t g_dsd_clock_hz;

uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev,
                                     USBD_AUDIO_ItfTypeDef *fops);
void USBD_AUDIO_Sync(USBD_HandleTypeDef *pdev);

#define CLAMP(x, y, z) ((x) < (y) ? (y) : (x) > (z) ? (z) : (x))

#ifdef __cplusplus
}
#endif

#endif /* __USB_AUDIO_H */
