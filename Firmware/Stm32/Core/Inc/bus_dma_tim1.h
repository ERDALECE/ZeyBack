#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Init with TIM1 clock frequency (Hz), e.g. 240000000
void     bus_dma_tim1_init(uint32_t tim1_clk_hz);

// Set sample rate (byte rate = 6*Fs). Can be 44100/48000/96000/192000/384000 etc.
void     bus_dma_tim1_set_fs(uint32_t fs_hz);

void     bus_dma_tim1_start(void);
void     bus_dma_tim1_stop(void);

uint32_t bus_dma_tim1_get_underrun(void);

// Optional: ppm trim (-300..+300). Used by ARR-sequence generator on next refill.
void     bus_dma_tim1_set_trim_ppm(int32_t ppm);

// Optional: call periodically (e.g. every 5ms) if you want slow drift control based on ring level.
void     bus_dma_tim1_pll_task(void);

typedef struct {
    uint32_t running;
    uint32_t fs_hz;
    uint32_t ticks_per_byte;
    uint32_t underrun;

    // Stream0 = TIM1_CH2 -> DATA
    uint32_t s0_ht_cnt;
    uint32_t s0_tc_cnt;
    uint32_t s0_err_cnt;
    uint32_t s0_last_lisr;   // raw DMA1->LISR snapshot
    uint32_t s0_err_code;    // HAL DMA ErrorCode
    uint32_t s0_cr;
    uint32_t s0_ndtr;

    // (Other streams run without IRQ; keep fields if you want to extend later)
    uint32_t s1_ht_cnt;
    uint32_t s1_tc_cnt;
    uint32_t s1_err_cnt;
    uint32_t s1_last_lisr;
    uint32_t s1_err_code;
    uint32_t s1_cr;
    uint32_t s1_ndtr;
} bus_dma_tim1_status_t;

void bus_dma_tim1_get_status(bus_dma_tim1_status_t *st);

#ifdef __cplusplus
}
#endif
