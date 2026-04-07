#include "audio_buffer.h"
#include "stm32h7xx.h"
#include <string.h>
#include "usbd_audio.h"
/*============================================================================
 * Backing memory
 *============================================================================*/

#define AB_FRAME_BYTES 8u
#define AB_CAPACITY_BYTES (AUDRB_SIZE * AB_FRAME_BYTES)

/*
 * Experimental DSD512 build:
 *  shadow area must be large enough for one full microframe = 1416 bytes
 */
__attribute__((section(".RAM_D2"), aligned(32)))
uint8_t usb_rx_buf[USB_RX_MAX_PACKET_SIZE];

__attribute__((section(".RAM_D2"), aligned(32)))
static uint8_t s_ab_mem[AB_CAPACITY_BYTES + USB_RX_MAX_PACKET_SIZE];

static AudioBuffer s_ab;

/* 2=unknown, 1=left-justified (LSB==0), 0=right-justified */
static volatile uint8_t g_s32_left_just = 2u;

static inline uint32_t load_u32_le_unaligned(const uint8_t *p)
{
    return *(const uint32_t *)(const void *)p;
}

static inline int32_t s24_from_s32_auto(int32_t v32)
{
    if (g_s32_left_just == 1u) {
        return v32 >> 8;
    } else if (g_s32_left_just == 0u) {
        return v32;
    }
    return v32 >> 8;
}

AudioBuffer* AudioBuffer_Instance(void)
{
    return &s_ab;
}

uint8_t AudioBuffer_GetState(void)
{
    return s_ab.state;
}

void AudioBuffer_Reset(AudioBuffer* ab)
{
    ab->state = AB_OK;
    ab->size  = 0u;
    ab->wr_ptr = 0u;
    ab->rd_ptr = 0u;
}

void AudioBuffer_Init(AudioBuffer* ab, uint32_t capacity_bytes)
{
    (void)capacity_bytes;
    ab->mem = s_ab_mem;
    ab->capacity = AB_CAPACITY_BYTES;
    AudioBuffer_Reset(ab);
    g_s32_left_just = 2u;
}

uint32_t AudioBuffer_LevelBytes(AudioBuffer* ab)
{
    return ab->size;
}

uint32_t AudioBuffer_FreeBytes(AudioBuffer* ab)
{
    if (ab->size >= ab->capacity) return 0u;
    return ab->capacity - ab->size;
}

void AudioBuffer_Receive(AudioBuffer* ab, uint32_t rxSize)
{
    if (rxSize == 0u) return;

    {
        uint32_t sz = ab->size;
        if (sz + rxSize > ab->capacity) {
            ab->size = ab->capacity;
            ab->state = AB_OVFL;
            return;
        }

        ab->size = sz + rxSize;
    }

    {
        uint32_t wp = ab->wr_ptr + rxSize;

        if (wp >= ab->capacity) {
            wp -= ab->capacity;

            if (wp > 0u) {
                memcpy(&ab->mem[0], &ab->mem[ab->capacity], wp);
            }
        }

        ab->wr_ptr = wp;
    }

    ab->state = AB_OK;
}

bool AudioBuffer_PopS24(AudioBuffer* ab, lr_sample_t* out)
{
    if (ab->size < AB_FRAME_BYTES) {
        ab->size = 0u;
        ab->state = AB_UDFL;
        return false;
    }

    if (g_s32_left_just == 2u) {
        const uint8_t *p0 = &ab->mem[ab->rd_ptr];
        uint32_t a = load_u32_le_unaligned(p0 + 0);
        uint32_t b = load_u32_le_unaligned(p0 + 4);
        uint32_t lsb = (a & 0xFFu) | (b & 0xFFu);
        g_s32_left_just = (lsb == 0u) ? 1u : 0u;
    }

    {
        const uint8_t *p = &ab->mem[ab->rd_ptr];
        int32_t l32 = (int32_t)load_u32_le_unaligned(p + 0);
        int32_t r32 = (int32_t)load_u32_le_unaligned(p + 4);

        out->l = s24_from_s32_auto(l32);
        out->r = s24_from_s32_auto(r32);
    }

    {
        uint32_t rp = ab->rd_ptr + AB_FRAME_BYTES;
        if (rp >= ab->capacity) rp -= ab->capacity;
        ab->rd_ptr = rp;
    }

    ab->size  -= AB_FRAME_BYTES;
    ab->state = AB_OK;
    return true;
}

/*============================================================================
 * Compatibility API
 *============================================================================*/

void audrb_init(void)
{
    AudioBuffer_Init(&s_ab, AB_CAPACITY_BYTES);
}

uint32_t audrb_count(void)
{
    return (uint32_t)(s_ab.size / AB_FRAME_BYTES);
}

uint32_t audrb_free(void)
{
    return (uint32_t)(AudioBuffer_FreeBytes(&s_ab) / AB_FRAME_BYTES);
}

uint32_t audrb_level_samples(void)
{
    return audrb_count();
}

uint32_t audrb_capacity_samples(void)
{
    return (uint32_t)(s_ab.capacity / AB_FRAME_BYTES);
}

bool audrb_push(lr_sample_t s)
{
    if (AudioBuffer_FreeBytes(&s_ab) < AB_FRAME_BYTES) {
        s_ab.state = AB_OVFL;
        return false;
    }

    {
        uint8_t *p = &s_ab.mem[s_ab.wr_ptr];
        int32_t l32 = (s.l << 8);
        int32_t r32 = (s.r << 8);

        memcpy(p + 0, &l32, 4);
        memcpy(p + 4, &r32, 4);
    }

    AudioBuffer_Receive(&s_ab, AB_FRAME_BYTES);
    return true;
}

bool audrb_pop(lr_sample_t* s)
{
    return AudioBuffer_PopS24(&s_ab, s);
}
