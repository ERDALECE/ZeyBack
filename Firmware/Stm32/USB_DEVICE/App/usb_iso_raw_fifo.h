#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Raw isoch OUT packet FIFO (fixed-size slots).
// USB callback enqueues quickly (copy), main loop drains and parses.

#ifndef ISO_RAW_MAX_PKT
#define ISO_RAW_MAX_PKT   512u
#endif

#ifndef ISO_RAW_SLOTS
#define ISO_RAW_SLOTS     64u   // 64*512 = 32KB
#endif

typedef struct {
    uint16_t len;
    uint16_t _rsv;
    uint8_t  data[ISO_RAW_MAX_PKT];
} iso_raw_slot_t;

void     iso_raw_fifo_init(void);
int      iso_raw_fifo_push_copy(const uint8_t *src, uint16_t len); // 1=ok,0=drop
int      iso_raw_fifo_pop(iso_raw_slot_t *out);                    // 1=ok,0=empty
uint32_t iso_raw_fifo_drop_count(void);
uint32_t iso_raw_fifo_level(void);

#ifdef __cplusplus
}
#endif
