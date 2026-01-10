#include "usb_iso_raw_fifo.h"

// Put in D2 if you like; adjust section name to your linker script.
__attribute__((section(".RAM_D2"), aligned(32)))
static volatile uint32_t g_wr = 0, g_rd = 0;

__attribute__((section(".RAM_D2"), aligned(32)))
static iso_raw_slot_t g_slots[ISO_RAW_SLOTS];

static volatile uint32_t g_drop = 0;

void iso_raw_fifo_init(void)
{
    g_wr = g_rd = 0;
    g_drop = 0;
}

static inline uint32_t next_idx(uint32_t x) { return (x + 1u) % ISO_RAW_SLOTS; }

uint32_t iso_raw_fifo_drop_count(void) { return g_drop; }

uint32_t iso_raw_fifo_level(void)
{
    uint32_t wr = g_wr, rd = g_rd;
    if (wr >= rd) return (wr - rd);
    return (ISO_RAW_SLOTS - rd + wr);
}

int iso_raw_fifo_push_copy(const uint8_t *src, uint16_t len)
{
    if (!src) return 0;
    if (len > ISO_RAW_MAX_PKT) { g_drop++; return 0; }

    uint32_t wr = g_wr;
    uint32_t nwr = next_idx(wr);
    if (nwr == g_rd) { g_drop++; return 0; } // full

    g_slots[wr].len = len;
    for (uint16_t i = 0; i < len; i++) {
        g_slots[wr].data[i] = src[i];
    }

    g_wr = nwr;
    return 1;
}

int iso_raw_fifo_pop(iso_raw_slot_t *out)
{
    if (!out) return 0;
    uint32_t rd = g_rd;
    if (rd == g_wr) return 0; // empty

    out->len = g_slots[rd].len;
    for (uint16_t i = 0; i < out->len; i++) {
        out->data[i] = g_slots[rd].data[i];
    }

    g_rd = next_idx(rd);
    return 1;
}
