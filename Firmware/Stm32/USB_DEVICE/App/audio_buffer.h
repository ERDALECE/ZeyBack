#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "usbd_audio.h"

#ifndef AUDRB_SIZE
#define AUDRB_SIZE 8192u   // 8192/16384/32768 deneyebilirsin
#endif

#if (AUDRB_SIZE & (AUDRB_SIZE - 1u)) != 0u
#error "AUDRB_SIZE must be power of two"
#endif

#define RB_MASK (AUDRB_SIZE - 1u)

extern __attribute__((section(".RAM_D2"), aligned(32)))
uint8_t usb_rx_buf[USB_HS_MAX_PACKET_SIZE];

typedef struct {
    int32_t l;
    int32_t r;
} lr_sample_t;

// power-of-two sized ring (audio_ring.c içinde AUDRB_SIZE)
void     audrb_init(void);
bool     audrb_push(lr_sample_t s);
bool     audrb_pop(lr_sample_t* s);
uint32_t audrb_count(void);
uint32_t audrb_free(void);
uint32_t audrb_level_samples(void);
uint32_t audrb_capacity_samples(void);
