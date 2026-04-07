/**
  ******************************************************************************
  * @file    audio_desc.c
  * @brief   USB Audio Class 2.0 Descriptors (PCM + DoP only)
  *
  * Experimental DSD512 build:
  *  - HS Iso OUT high-bandwidth enabled for 1416 byte/microframe
  ******************************************************************************
  */

#include "audio_desc.h"
#include "usbd_audio.h"

/*
 * Descriptor size calculation:
 * Config: 9
 * IAD: 8
 * AC Interface: 9 + 9 + 8 + 17 + 14 + 12 = 69
 * AS PCM Interface: 9 + 9 + 16 + 6 + 7 + 8 + 7 = 62
 * Total: 9 + 8 + 69 + 62 = 148 bytes
 */
#define USB_AUDIO_CONFIG_DESC_SZ  148U

const uint8_t USBD_AUDIO_CfgDesc[USB_AUDIO_CONFIG_DESC_SZ] __attribute__ ((aligned (4))) =
{
    /*=========================================================================
     * Configuration Descriptor
     *=========================================================================*/
    0x09,
    USB_DESC_TYPE_CONFIGURATION,
    LOBYTE(USB_AUDIO_CONFIG_DESC_SZ),
    HIBYTE(USB_AUDIO_CONFIG_DESC_SZ),
    0x02,
    0x01,
    0x00,
#if (USBD_SELF_POWERED == 1U)
    0xC0,
#else
    0x80,
#endif
    USBD_MAX_POWER,

    /*=========================================================================
     * IAD: Interface Association Descriptor
     *=========================================================================*/
    0x08,
    USB_DESC_TYPE_IAD,
    0x00,
    0x02,
    0x01,
    0x00,
    0x20,
    0x00,

    /*=========================================================================
     * Interface 0: Audio Control
     *=========================================================================*/
    0x09,
    USB_DESC_TYPE_INTERFACE,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x20,
    0x00,

    /* Class-Specific AC Interface Header Descriptor */
    0x09,
    0x24,
    0x01,
    0x00, 0x02,
    0x00,
    LOBYTE(60),
    HIBYTE(60),
    0x00,

    /* Clock Source Descriptor */
    0x08,
    0x24,
    0x0A,
    0x04,
    0x03,
    0x03,
    0x00,
    0x00,

    /* Input Terminal Descriptor */
    0x11,
    0x24,
    0x02,
    0x01,
    0x01, 0x01,
    0x00,
    0x04,
    0x02,
    0x03, 0x00, 0x00, 0x00,
    0x00,
    0x00, 0x00,
    0x00,

    /* Feature Unit Descriptor */
    0x0E,
    0x24,
    0x06,
    0x02,
    0x01,
    0x0F, 0x00, 0x00, 0x00,
    0x0F, 0x00, 0x00, 0x00,
    0x00,

    /* Output Terminal Descriptor */
    0x0C,
    0x24,
    0x03,
    0x03,
    0x01, 0x03,
    0x00,
    0x02,
    0x04,
    0x00, 0x00,
    0x00,

    /*=========================================================================
     * Interface 1: Audio Streaming (PCM + DoP)
     *=========================================================================*/
    /* Alt Setting 0: Zero Bandwidth */
    0x09,
    USB_DESC_TYPE_INTERFACE,
    0x01,
    0x00,
    0x00,
    0x01,
    0x02,
    0x20,
    0x00,

    /* Alt Setting 1: PCM 32-bit Stereo */
    0x09,
    USB_DESC_TYPE_INTERFACE,
    0x01,
    0x01,
    0x02,
    0x01,
    0x02,
    0x20,
    0x00,

    /* AS Class-Specific General Descriptor */
    0x10,
    0x24,
    0x01,
    0x01,
    0x05,
    0x01,
    0x01, 0x00, 0x00, 0x00,
    0x02,
    0x03, 0x00, 0x00, 0x00,
    0x00,

    /* Format Type I Descriptor */
    0x06,
    0x24,
    0x02,
    0x01,
    0x04,
    0x18,

    /* PCM/DoP Data Endpoint Descriptor */
    0x07,
    USB_DESC_TYPE_ENDPOINT,
    0x01,                                   /* OUT EP1 */
    0x05,                                   /* Isochronous, Async */

    /*
     * HS high-bandwidth endpoint:
     *   0x0AC4 = 708 bytes/transaction, 2 transactions/microframe
     *          = 1416 bytes/microframe total
     *
     * This is required for DoP DSD512 carrier (1411200 Hz):
     *   177 stereo frames * 8 bytes = 1416 bytes / microframe
     */
    LOBYTE(USB_AUDIO_OUT_EP_WMAXPACKET_HS),
    HIBYTE(USB_AUDIO_OUT_EP_WMAXPACKET_HS),
    0x01,

    /* AS Endpoint Class-Specific Descriptor */
    0x08,
    0x25,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00, 0x00,

    /* Feedback Endpoint Descriptor */
    0x07,
    USB_DESC_TYPE_ENDPOINT,
    0x81,
    0x11,
    0x04, 0x00,
    0x04,
};

const uint16_t USBD_AUDIO_CfgDescSize = USB_AUDIO_CONFIG_DESC_SZ;

const uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __attribute__ ((aligned (4))) =
{
    USB_LEN_DEV_QUALIFIER_DESC,
    USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00, 0x02,
    0xEF,
    0x02,
    0x01,
    USB_MAX_EP0_SIZE,
    0x00,
    0x00,
};
