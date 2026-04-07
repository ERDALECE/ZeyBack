#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "usbd_audio.h"  // USB_HS_MAX_PACKET_SIZE

// ============================================================
// Audio RX ring (USB OUT -> consumer)
//
// Goal:
//  - Keep your existing audrb_* API so bus_dma_tim1.c doesn't change.
//  - Use an OpenUAC2-like byte ring underneath so we can later switch
//    to true zero-copy / direct USB writes if needed.
//
// Storage format inside ring:
//  - USB PCM S32_LE stereo frames, 8 bytes per frame (L32, R32).
//  - audrb_pop() converts to signed 24-bit in int32 (by >>8 when left-just).
//
// NOTE:
//  AUDRB_SIZE is the number of stereo frames in the ring (power-of-two).
// ============================================================

#ifndef AUDRB_SIZE
#define AUDRB_SIZE 32768u
#endif

#if (AUDRB_SIZE & (AUDRB_SIZE - 1u)) != 0u
#error "AUDRB_SIZE must be power of two"
#endif

#define RB_MASK (AUDRB_SIZE - 1u)

// Old type kept for compatibility
typedef struct {
    int32_t l;
    int32_t r;
} lr_sample_t;

// --- OpenUAC2-style state
typedef enum {
    AB_OK = 0,
    AB_OVFL = 1,
    AB_UDFL = 2
} AudioBufferState;

typedef struct {
    uint8_t *mem;       // backing memory
    uint32_t capacity;  // bytes (must be power-of-two * 8)
    volatile uint32_t size;    // bytes currently buffered
    volatile uint32_t wr_ptr;  // byte index
    volatile uint32_t rd_ptr;  // byte index
    volatile uint8_t state;    // AudioBufferState
} AudioBuffer;

// Singleton instance (like OpenUAC2)
AudioBuffer* AudioBuffer_Instance(void);
uint8_t      AudioBuffer_GetState(void);
void         AudioBuffer_Init(AudioBuffer* ab, uint32_t capacity_bytes);
void         AudioBuffer_Reset(AudioBuffer* ab);
void         AudioBuffer_Receive(AudioBuffer* ab, uint32_t rxSize);

// Read one S32_LE stereo frame (8 bytes) and convert to 24-bit sample
bool         AudioBuffer_PopS24(AudioBuffer* ab, lr_sample_t* out);

// Helpful metrics
uint32_t     AudioBuffer_LevelBytes(AudioBuffer* ab);
uint32_t     AudioBuffer_FreeBytes(AudioBuffer* ab);

#define AudioBuffer_WrPtr(ab) (&(ab)->mem[(ab)->wr_ptr])


// ============================================================
// Compatibility API used by existing code
// ============================================================
void     audrb_init(void);
bool     audrb_push(lr_sample_t s);      // packs into S32_LE, rarely used (tests)
bool     audrb_pop(lr_sample_t* s);      // returns 24-bit in int32
uint32_t audrb_count(void);              // frames
uint32_t audrb_free(void);               // frames
uint32_t audrb_level_samples(void);
uint32_t audrb_capacity_samples(void);

// USB RX buffer is still used by USB stack as scratch for EP0 etc.
extern __attribute__((section(".RAM_D2"), aligned(32)))
uint8_t usb_rx_buf[USB_RX_MAX_PACKET_SIZE];

