// ========================== bus_dma_tim1.c (buffer + ARR DMA fixed) ==========================
#include "bus_dma_tim1.h"
#include "audio_buffer.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx.h"   // DMA_Stream_TypeDef
#include <string.h>
#include <stdint.h>

/*
 * STM32H743 TIM1 + DMA GPIO bus driver
 *
 * GPIO:
 *   - PD8..PD15 : DATA[7:0]
 *   - PC6       : ACK (toggle each byte)
 *   - PC7       : SYNC (short pulse once per 6-byte sample)
 *
 * Timing (one TIM1 period = one byte):
 *   - CC2 early   : write DATA bus
 *   - CC3 early   : SYNC set  (only on byte0 of each sample)
 *   - CC4 early   : SYNC clear (only on byte0 of each sample) -> makes a short pulse
 *   - CC1 mid/late: ACK toggle (one edge per byte, FPGA tgl_evt = XOR)
 *   - UPDATE      : (optional) write TIM1->ARR from arr_seq (fractional trim, ppm-level)
 *
 * Notes:
 *   - Only Stream0 (DATA) uses interrupts (HT/TC) to refill buffers.
 *   - Other DMA streams run in circular mode without IRQ.
 *   - Buffers are placed in D1 (cacheable) to save D2. We CleanDCache on refill.
 */

// ===== GPIO mapping =====
#define DATA_GPIO        GPIOD
#define DATA_SHIFT       8u
#define DATA_MASK        (0xFFu << DATA_SHIFT)

#define CTRL_GPIO        GPIOC
#define ACK_PIN          (1u << 6)     // PC6
#define SYNC_PIN         (1u << 7)     // PC7

// ===== Buffer sizing =====
// NOTE: user said D2 RAM was tight. These buffers now live in D1, so you can raise this later if needed.
#define SAMPLES_PER_HALF    2048u
#define BYTES_PER_SAMPLE    6u
#define BYTES_PER_HALF      (SAMPLES_PER_HALF * BYTES_PER_SAMPLE)   // entries per half (1 entry = 1 byte)
#define BYTES_TOTAL         (2u * BYTES_PER_HALF)

// ===== Tunables =====
#define CC_DATA_TICKS            10u   // CC2
#define SYNC_MIN_WIDTH_TICKS     14u   // min pulse width in timer ticks
#define TRIM_MAX_PPM            300

// ===== Place TIM/DMA source buffers in D1 (cacheable) =====
// If you don't have a .RAM_D1 section, remove section() attribute and they will go to default .bss (usually D1).
__attribute__((section(".RAM_D1"), aligned(32)))
static uint32_t data_bsrr[BYTES_TOTAL];        // TIM1_CH2 -> GPIOD->BSRR

__attribute__((section(".RAM_D1"), aligned(32)))
static uint32_t ack_bsrr[BYTES_TOTAL];         // TIM1_CH1 -> GPIOC->BSRR (PC6 toggle)

__attribute__((section(".RAM_D1"), aligned(32)))
static uint32_t sync_set_bsrr[BYTES_TOTAL];    // TIM1_CH3 -> GPIOC->BSRR (PC7 set on byte0)

__attribute__((section(".RAM_D1"), aligned(32)))
static uint32_t sync_clr_bsrr[BYTES_TOTAL];    // TIM1_CH4 -> GPIOC->BSRR (PC7 clear on byte0)

// ARR sequence (ticks-1 per byte) for fractional trim (ppm-level).
// Each element is a halfword written to TIM1->ARR on UPDATE event.
__attribute__((section(".RAM_D1"), aligned(32)))
static uint16_t arr_seq[BYTES_TOTAL];

// ===== Peripherals =====
static TIM_HandleTypeDef htim1;

static DMA_HandleTypeDef hdma_tim1_ch2_data;       // DMA1_Stream0, TIM1_CH2 -> DATA
static DMA_HandleTypeDef hdma_tim1_ch1_ack;        // DMA1_Stream1, TIM1_CH1 -> ACK
static DMA_HandleTypeDef hdma_tim1_ch3_sync_set;   // DMA1_Stream2, TIM1_CH3 -> SYNC set
static DMA_HandleTypeDef hdma_tim1_ch4_sync_clr;   // DMA1_Stream3, TIM1_CH4 -> SYNC clr
static DMA_HandleTypeDef hdma_tim1_up_arr;         // DMA1_Stream4, TIM1_UP  -> TIM1->ARR (halfword)

// ===== Runtime state =====
static volatile uint32_t g_dma_running = 0;
static volatile uint32_t g_underrun = 0;
static uint32_t g_tim1_clk_hz = 0;
static uint32_t g_fs_hz = 48000;
static uint32_t g_ticks_per_byte = 100; // base integer ticks/byte
static uint32_t ack_state = 0;

static volatile bus_dma_tim1_status_t g_st;

// ppm trim (set by pll task)
static volatile int32_t g_trim_ppm = 0;

// ===== Low-level DMA register helpers =====
#define DMA_STREAM_PTR(hdma_)   ((DMA_Stream_TypeDef *)((hdma_)->Instance))
#define DMA_GET_NDTR(hdma_)     (DMA_STREAM_PTR(hdma_)->NDTR)
#define DMA_GET_CR(hdma_)       (DMA_STREAM_PTR(hdma_)->CR)

// ===== DCache clean helper (align address+len to 32B lines) =====
static inline void dcache_clean_range(void *p, uint32_t len)
{
#if defined(SCB_CleanDCache_by_Addr)
    uintptr_t a  = (uintptr_t)p;
    uintptr_t a0 = a & ~(uintptr_t)31;
    uintptr_t a1 = (a + (uintptr_t)len + 31u) & ~(uintptr_t)31;
    SCB_CleanDCache_by_Addr((uint32_t*)a0, (int32_t)(a1 - a0));
#else
    (void)p; (void)len;
#endif
}

// ======================= Status API =======================
void bus_dma_tim1_get_status(bus_dma_tim1_status_t *st)
{
    if (!st) return;
    __disable_irq();
    g_st.running        = g_dma_running;
    g_st.fs_hz          = g_fs_hz;
    g_st.ticks_per_byte = g_ticks_per_byte;
    g_st.underrun       = g_underrun;
    *st = (bus_dma_tim1_status_t)g_st;
    __enable_irq();
}

uint32_t bus_dma_tim1_get_underrun(void) { return g_underrun; }

// ======================= Helpers =======================
static inline uint32_t pack_data_bsrr(uint8_t b)
{
    uint32_t set = (((uint32_t)b << DATA_SHIFT) & DATA_MASK);
    uint32_t rst = DATA_MASK;
    return (rst << 16) | set;
}

static inline uint32_t pack_ack_bsrr_toggle(void)
{
    uint32_t w = (!ack_state) ? ACK_PIN : (ACK_PIN << 16);
    ack_state ^= 1u;
    return w;
}

// Build ARR half based on base ticks/byte + ppm trim.
// offset_entries: 0 or BYTES_PER_HALF
static void build_arr_half(uint32_t offset_entries)
{
    // base ticks/byte in Q16.16
    uint32_t base_q16 = (g_ticks_per_byte << 16);

    // ppm adjustment: base * ppm / 1e6  (signed)
    int32_t ppm = (int32_t)g_trim_ppm;
    if (ppm >  TRIM_MAX_PPM) ppm =  TRIM_MAX_PPM;
    if (ppm < -TRIM_MAX_PPM) ppm = -TRIM_MAX_PPM;

    int32_t adj_q16 = (int32_t)(((int64_t)base_q16 * (int64_t)ppm) / 1000000LL);
    int32_t tpb_q16 = (int32_t)base_q16 + adj_q16;

    // Safety clamp (ticks/byte must stay >= 32)
    int32_t min_q16 = (int32_t)(32u << 16);
    if (tpb_q16 < min_q16) tpb_q16 = min_q16;

    uint32_t t_int  = (uint32_t)((uint32_t)tpb_q16 >> 16);
    uint32_t t_frac = (uint32_t)tpb_q16 & 0xFFFFu;

    // Simple fractional dithering: occasionally add 1 tick to meet average
    uint32_t acc = 0;
    for (uint32_t i = 0; i < BYTES_PER_HALF; i++) {
        uint32_t ticks = t_int;
        acc += t_frac;
        if (acc >= 0x10000u) { acc -= 0x10000u; ticks++; }
        arr_seq[offset_entries + i] = (uint16_t)(ticks - 1u);
    }
}

#ifdef pattern_test
// Underrun: 1 kHz square (Left), Right=0.
static inline void make_underrun_test_lr(uint32_t si, uint32_t fs_hz, lr_sample_t *out)
{
    uint32_t half = (fs_hz >= 2000u) ? (fs_hz / 2000u) : 1u;
    uint32_t ph   = (si % (2u * half));
    int32_t  Ls   = (ph < half) ? 0x007FFFFF : (int32_t)0xFF800000;
    out->l = Ls;
    out->r = 0;
}
#endif

// Fill half: offset_entries = 0 or BYTES_PER_HALF
static void fill_half(uint32_t offset_entries)
{
    // Build ARR half first (so DMA sees fresh ARR values)
    build_arr_half(offset_entries);

    lr_sample_t s;
    for (uint32_t si = 0; si < SAMPLES_PER_HALF; si++)
    {
        uint8_t b[6];
#ifndef pattern_test
        static lr_sample_t last = {0};
        if (audrb_pop(&s))
        {
            uint32_t L = (uint32_t)(s.l) & 0xFFFFFFu;
            uint32_t R = (uint32_t)(s.r) & 0xFFFFFFu;
            last = s;
            // Big-endian (MSB first)
            b[0] = (uint8_t)(L >> 16);
            b[1] = (uint8_t)(L >>  8);
            b[2] = (uint8_t)(L >>  0);
            b[3] = (uint8_t)(R >> 16);
            b[4] = (uint8_t)(R >>  8);
            b[5] = (uint8_t)(R >>  0);
        }
        else
        {
            // ZOH on underrun (no test tone)
            s = last;
            uint32_t L = (uint32_t)(s.l) & 0xFFFFFFu;
            uint32_t R = (uint32_t)(s.r) & 0xFFFFFFu;
            b[0] = (uint8_t)(L >> 16);
            b[1] = (uint8_t)(L >>  8);
            b[2] = (uint8_t)(L >>  0);
            b[3] = (uint8_t)(R >> 16);
            b[4] = (uint8_t)(R >>  8);
            b[5] = (uint8_t)(R >>  0);
        }
#else
        g_underrun++;
        make_underrun_test_lr(si, g_fs_hz, &s);
        uint32_t L = ((uint32_t)s.l) & 0xFFFFFFu;
        uint32_t R = ((uint32_t)s.r) & 0xFFFFFFu;
        b[0] = (uint8_t)(L >> 16);
        b[1] = (uint8_t)(L >>  8);
        b[2] = (uint8_t)(L >>  0);
        b[3] = (uint8_t)(R >> 16);
        b[4] = (uint8_t)(R >>  8);
        b[5] = (uint8_t)(R >>  0);
#endif

        uint32_t base = offset_entries + si * 6u;
        for (uint32_t bi = 0; bi < 6u; bi++)
        {
            // DATA every byte
            data_bsrr[base + bi] = pack_data_bsrr(b[bi]);

            // ACK toggle every byte
            ack_bsrr[base + bi]  = pack_ack_bsrr_toggle();

            // SYNC pulse only at sample start (byte0)
            if (bi == 0u) {
                sync_set_bsrr[base + bi] = SYNC_PIN;
                sync_clr_bsrr[base + bi] = (SYNC_PIN << 16);
            } else {
                sync_set_bsrr[base + bi] = 0u;
                sync_clr_bsrr[base + bi] = 0u;
            }
        }
    }

    // Clean caches for this half (D1 is cacheable)
    dcache_clean_range(&data_bsrr[offset_entries],     BYTES_PER_HALF * sizeof(uint32_t));
    dcache_clean_range(&ack_bsrr[offset_entries],      BYTES_PER_HALF * sizeof(uint32_t));
    dcache_clean_range(&sync_set_bsrr[offset_entries], BYTES_PER_HALF * sizeof(uint32_t));
    dcache_clean_range(&sync_clr_bsrr[offset_entries], BYTES_PER_HALF * sizeof(uint32_t));
    dcache_clean_range(&arr_seq[offset_entries],       BYTES_PER_HALF * sizeof(uint16_t));
}

static void gpio_init_bus(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gi = {0};
    gi.Mode  = GPIO_MODE_OUTPUT_PP;
    gi.Pull  = GPIO_NOPULL;
    gi.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    // PD8..15
    gi.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|
             GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &gi);

    // PC6, PC7
    gi.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOC, &gi);

    // default low
    CTRL_GPIO->BSRR = ((ACK_PIN | SYNC_PIN) << 16);
    DATA_GPIO->BSRR = (DATA_MASK << 16);
}

static void dma_init_one(DMA_HandleTypeDef *hdma,
                         DMA_Stream_TypeDef *stream,
                         uint32_t request,
                         uint32_t priority,
                         uint32_t periph_align,
                         uint32_t mem_align)
{
    hdma->Instance                 = stream;
    hdma->Init.Request             = request;
    hdma->Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma->Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma->Init.MemInc              = DMA_MINC_ENABLE;
    hdma->Init.PeriphDataAlignment = periph_align;
    hdma->Init.MemDataAlignment    = mem_align;
    hdma->Init.Mode                = DMA_CIRCULAR;
    hdma->Init.Priority            = priority;
    hdma->Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    (void)HAL_DMA_Init(hdma);
}

static void dmamux_dma_init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();
#if defined(__HAL_RCC_DMAMUX1_CLK_ENABLE)
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
#elif defined(RCC_AHB1ENR_DMAMUX1EN)
    RCC->AHB1ENR |= RCC_AHB1ENR_DMAMUX1EN;
    __DSB();
#endif
    __HAL_RCC_TIM1_CLK_ENABLE();

    // TIM1_CH2 -> DATA (word)
    dma_init_one(&hdma_tim1_ch2_data, DMA1_Stream0, DMA_REQUEST_TIM1_CH2,
                 DMA_PRIORITY_VERY_HIGH, DMA_PDATAALIGN_WORD, DMA_MDATAALIGN_WORD);
    __HAL_LINKDMA(&htim1, hdma[TIM_DMA_ID_CC2], hdma_tim1_ch2_data);

    // TIM1_CH1 -> ACK (word)
    dma_init_one(&hdma_tim1_ch1_ack, DMA1_Stream1, DMA_REQUEST_TIM1_CH1,
                 DMA_PRIORITY_HIGH, DMA_PDATAALIGN_WORD, DMA_MDATAALIGN_WORD);
    __HAL_LINKDMA(&htim1, hdma[TIM_DMA_ID_CC1], hdma_tim1_ch1_ack);

    // TIM1_CH3 -> SYNC set (word)
    dma_init_one(&hdma_tim1_ch3_sync_set, DMA1_Stream2, DMA_REQUEST_TIM1_CH3,
                 DMA_PRIORITY_HIGH, DMA_PDATAALIGN_WORD, DMA_MDATAALIGN_WORD);
    __HAL_LINKDMA(&htim1, hdma[TIM_DMA_ID_CC3], hdma_tim1_ch3_sync_set);

    // TIM1_CH4 -> SYNC clear (word)
    dma_init_one(&hdma_tim1_ch4_sync_clr, DMA1_Stream3, DMA_REQUEST_TIM1_CH4,
                 DMA_PRIORITY_HIGH, DMA_PDATAALIGN_WORD, DMA_MDATAALIGN_WORD);
    __HAL_LINKDMA(&htim1, hdma[TIM_DMA_ID_CC4], hdma_tim1_ch4_sync_clr);

    // TIM1_UP -> ARR (halfword)
    dma_init_one(&hdma_tim1_up_arr, DMA1_Stream4, DMA_REQUEST_TIM1_UP,
                 DMA_PRIORITY_HIGH, DMA_PDATAALIGN_HALFWORD, DMA_MDATAALIGN_HALFWORD);
    __HAL_LINKDMA(&htim1, hdma[TIM_DMA_ID_UPDATE], hdma_tim1_up_arr);

    // IRQ: only Stream0 (DATA)
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 1, 0); // USB stays at prio 0
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

static void tim1_init(uint32_t ticks_per_byte)
{
    // One period per byte
    htim1.Instance = TIM1;
    htim1.Init.Prescaler         = 0;
    htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1.Init.Period            = (ticks_per_byte - 1u);
    htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    // IMPORTANT: enable preload so ARR updates (via DMA) take effect cleanly at update boundaries
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    (void)HAL_TIM_Base_Init(&htim1);
    (void)HAL_TIM_OC_Init(&htim1);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode     = TIM_OCMODE_TIMING;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;

    // CC2: DATA very early
    uint32_t cc2 = CC_DATA_TICKS;
    if (cc2 >= (ticks_per_byte - 1u)) cc2 = 1u;
    oc.Pulse = cc2;
    (void)HAL_TIM_OC_ConfigChannel(&htim1, &oc, TIM_CHANNEL_2);

    // CC1: ACK mid/late
    uint32_t cc1 = (ticks_per_byte / 2u);
    if (cc1 < 2u) cc1 = 2u;
    if (cc1 >= (ticks_per_byte - 2u)) cc1 = (ticks_per_byte / 3u);
    oc.Pulse = cc1;
    (void)HAL_TIM_OC_ConfigChannel(&htim1, &oc, TIM_CHANNEL_1);

    // CC3/CC4: SYNC pulse early, before ACK
    uint32_t cc3 = (ticks_per_byte / 8u);
    if (cc3 < 8u) cc3 = 8u;
    if (cc3 <= cc2 + 2u) cc3 = cc2 + 4u;

    uint32_t width = (ticks_per_byte / 16u);
    if (width < SYNC_MIN_WIDTH_TICKS) width = SYNC_MIN_WIDTH_TICKS;

    uint32_t cc4 = cc3 + width;

    // ensure cc4 < cc1
    if (cc4 + 4u >= cc1) {
        if (cc1 > 16u) {
            cc4 = (cc1 > 8u) ? (cc1 - 8u) : (cc1 - 2u);
            if (cc4 <= cc3 + 2u) {
                cc3 = (cc4 > 8u) ? (cc4 - 8u) : 2u;
            }
        } else {
            cc3 = 2u;
            cc4 = 4u;
        }
    }

    // Final clamp to ARR
    uint32_t arr = ticks_per_byte - 1u;
    if (cc3 >= arr) cc3 = (arr > 4u) ? 4u : 1u;
    if (cc4 >= arr) cc4 = (arr > 8u) ? 8u : 2u;

    oc.Pulse = cc3;
    (void)HAL_TIM_OC_ConfigChannel(&htim1, &oc, TIM_CHANNEL_3);

    oc.Pulse = cc4;
    (void)HAL_TIM_OC_ConfigChannel(&htim1, &oc, TIM_CHANNEL_4);

    // enable compare events
    TIM1->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E);
}

// ===== Stream0 callbacks =====
static void s0_half_cb(DMA_HandleTypeDef *hdma)
{
    (void)hdma;
    if (!g_dma_running) return;
    g_st.s0_ht_cnt++;
    fill_half(0);
}

static void s0_full_cb(DMA_HandleTypeDef *hdma)
{
    (void)hdma;
    if (!g_dma_running) return;
    g_st.s0_tc_cnt++;
    fill_half(BYTES_PER_HALF);
}

static void s0_err_cb(DMA_HandleTypeDef *hdma)
{
    (void)hdma;
    g_st.s0_err_cnt++;
    g_st.s0_err_code  = hdma_tim1_ch2_data.ErrorCode;
    g_st.s0_last_lisr = DMA1->LISR;
    g_st.s0_cr        = DMA_GET_CR(&hdma_tim1_ch2_data);
    g_st.s0_ndtr      = DMA_GET_NDTR(&hdma_tim1_ch2_data);
}

// ======================= Public API =======================
void bus_dma_tim1_init(uint32_t tim1_clk_hz)
{
    memset((void*)&g_st, 0, sizeof(g_st));
    g_tim1_clk_hz = tim1_clk_hz;

    gpio_init_bus();
    dmamux_dma_init();

    // callbacks only for Stream0 (DATA)
    hdma_tim1_ch2_data.XferHalfCpltCallback = s0_half_cb;
    hdma_tim1_ch2_data.XferCpltCallback     = s0_full_cb;
    hdma_tim1_ch2_data.XferErrorCallback    = s0_err_cb;

    bus_dma_tim1_set_fs(48000u);
}

void bus_dma_tim1_set_fs(uint32_t fs_hz)
{
    g_fs_hz = fs_hz;

    uint32_t fbyte = 6u * fs_hz;
    if (fbyte == 0u) fbyte = 1u;

    // nearest integer ticks per byte
    uint32_t t = (g_tim1_clk_hz + (fbyte/2u)) / fbyte;

    // give room for CC offsets even at high rates
    if (t < 32u) t = 32u;

    g_ticks_per_byte = t;

    if (g_dma_running) {
        bus_dma_tim1_stop();
        bus_dma_tim1_start();
    }
}

void bus_dma_tim1_start(void)
{
    ack_state  = 0;
    g_underrun = 0;

    // prefill
    fill_half(0);
    fill_half(BYTES_PER_HALF);

    tim1_init(g_ticks_per_byte);

    // Set initial ARR to first entry (preload)
    TIM1->ARR = (uint32_t)arr_seq[0];
    TIM1->EGR = TIM_EGR_UG;

    // Clear DMA flags for streams 0..3 (LIFCR) and stream4 (HIFCR)
    DMA1->LIFCR =
        (DMA_LIFCR_CFEIF0  | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0 |
         DMA_LIFCR_CFEIF1  | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTCIF1 |
         DMA_LIFCR_CFEIF2  | DMA_LIFCR_CDMEIF2 | DMA_LIFCR_CTEIF2 | DMA_LIFCR_CHTIF2 | DMA_LIFCR_CTCIF2 |
         DMA_LIFCR_CFEIF3  | DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CTEIF3 | DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTCIF3);

    DMA1->HIFCR =
        (DMA_HIFCR_CFEIF4  | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTCIF4);

    // Start DMA streams
    (void)HAL_DMA_Start_IT(&hdma_tim1_ch2_data,
                          (uint32_t)&data_bsrr[0],
                          (uint32_t)&DATA_GPIO->BSRR,
                          BYTES_TOTAL);

    (void)HAL_DMA_Start(&hdma_tim1_ch1_ack,
                        (uint32_t)&ack_bsrr[0],
                        (uint32_t)&CTRL_GPIO->BSRR,
                        BYTES_TOTAL);

    (void)HAL_DMA_Start(&hdma_tim1_ch3_sync_set,
                        (uint32_t)&sync_set_bsrr[0],
                        (uint32_t)&CTRL_GPIO->BSRR,
                        BYTES_TOTAL);

    (void)HAL_DMA_Start(&hdma_tim1_ch4_sync_clr,
                        (uint32_t)&sync_clr_bsrr[0],
                        (uint32_t)&CTRL_GPIO->BSRR,
                        BYTES_TOTAL);

    // ARR update stream (halfword)
    (void)HAL_DMA_Start(&hdma_tim1_up_arr,
                        (uint32_t)&arr_seq[0],
                        (uint32_t)&TIM1->ARR,
                        BYTES_TOTAL);

    // Enable TIM DMA requests for CC1/CC2/CC3/CC4 + UPDATE
    __HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_CC1);
    __HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_CC2);
    __HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_CC3);
    __HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_CC4);
    __HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_UPDATE);

    // Start timer + channels
    (void)HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_1);
    (void)HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_2);
    (void)HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_3);
    (void)HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_4);
    (void)HAL_TIM_Base_Start(&htim1);

    g_dma_running = 1;
}

void bus_dma_tim1_stop(void)
{
    g_dma_running = 0;

    (void)HAL_TIM_Base_Stop(&htim1);
    (void)HAL_TIM_OC_Stop(&htim1, TIM_CHANNEL_1);
    (void)HAL_TIM_OC_Stop(&htim1, TIM_CHANNEL_2);
    (void)HAL_TIM_OC_Stop(&htim1, TIM_CHANNEL_3);
    (void)HAL_TIM_OC_Stop(&htim1, TIM_CHANNEL_4);

    __HAL_TIM_DISABLE_DMA(&htim1, TIM_DMA_CC1);
    __HAL_TIM_DISABLE_DMA(&htim1, TIM_DMA_CC2);
    __HAL_TIM_DISABLE_DMA(&htim1, TIM_DMA_CC3);
    __HAL_TIM_DISABLE_DMA(&htim1, TIM_DMA_CC4);
    __HAL_TIM_DISABLE_DMA(&htim1, TIM_DMA_UPDATE);

    (void)HAL_DMA_Abort(&hdma_tim1_ch2_data);
    (void)HAL_DMA_Abort(&hdma_tim1_ch1_ack);
    (void)HAL_DMA_Abort(&hdma_tim1_ch3_sync_set);
    (void)HAL_DMA_Abort(&hdma_tim1_ch4_sync_clr);
    (void)HAL_DMA_Abort(&hdma_tim1_up_arr);

    // pins low
    CTRL_GPIO->BSRR = ((ACK_PIN | SYNC_PIN) << 16);
    DATA_GPIO->BSRR = (DATA_MASK << 16);
}

// ===== IRQ (only Stream0 enabled) =====
void DMA1_Stream0_IRQHandler(void)
{
    g_st.s0_last_lisr = DMA1->LISR;
    g_st.s0_cr        = DMA_GET_CR(&hdma_tim1_ch2_data);
    g_st.s0_ndtr      = DMA_GET_NDTR(&hdma_tim1_ch2_data);
    HAL_DMA_IRQHandler(&hdma_tim1_ch2_data);
}

void bus_dma_tim1_set_trim_ppm(int32_t ppm)
{
    if (ppm >  TRIM_MAX_PPM) ppm =  TRIM_MAX_PPM;
    if (ppm < -TRIM_MAX_PPM) ppm = -TRIM_MAX_PPM;
    g_trim_ppm = ppm;
}

// Call this periodically (e.g. every 5 ms) from main loop / SysTick.
void bus_dma_tim1_pll_task(void)
{
    // You can implement your own audrb_level_samples()/capacity. If not available, do simple count.
    int32_t lvl = (int32_t)audrb_count();
    int32_t cap = (int32_t)RB_MASK;
    int32_t target = cap / 2;

    int32_t err = lvl - target;

    // deadband (%2)
    int32_t dead = cap / 50;
    if (err > dead) err -= dead;
    else if (err < -dead) err += dead;
    else err = 0;

    static int32_t ppm = 0;
    ppm += err / 400;   // gentle gain
    if (ppm > 200) ppm = 200;
    if (ppm < -200) ppm = -200;

    bus_dma_tim1_set_trim_ppm(ppm);
    // g_trim_ppm is used by build_arr_half() on next refill
}

// ======================== end bus_dma_tim1.c ========================
