
#include "main.h"
#include "audio_buffer.h"   // audrb_pop, lr_sample_t
#include "memorymap.h"
#include "gpio.h"
#include "core_cm7.h"       // __disable_irq / __enable_irq
#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "stm32_fpga.h"

 extern volatile uint32_t g_rb_drop;
 extern volatile uint32_t g_audio_ticks;     // FS kadar artar
 extern volatile uint32_t g_rb_drop;         // ring full -> drop frame
 volatile uint8_t g_audio_run = 0;
 volatile uint32_t g_rb_udf = 0;
 extern volatile uint32_t g_rb_underrun;
 extern volatile uint8_t  g_audio_run;

 void MX_GPIO_Init_AudioPins(void)
{
	    BUS_CLK_EN();

	    GPIO_InitTypeDef g = {0};

	    /* ---------- Outputs: PD3, PD4, PD5, PD8..PD15 ---------- */
	    g.Mode  = GPIO_MODE_OUTPUT_PP;
	    g.Pull  = GPIO_NOPULL;
	    g.Speed = GPIO_SPEED_FREQ_HIGH;

	    g.Pin = GPIO_PIN_4 |
	            GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
	            GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	    HAL_GPIO_Init(DATA_GPIO, &g);

	    /* ---------- Input: PD6 (READY) ---------- */
	    g.Mode = GPIO_MODE_INPUT;
	    g.Pull = GPIO_NOPULL;            // istersen PULLDOWN (FPGA open-drain ise) düşünebiliriz
	    g.Speed = GPIO_SPEED_FREQ_HIGH;
	    g.Pin  = GPIO_PIN_6;
	    HAL_GPIO_Init(DATA_GPIO, &g);

	    /* ---------- Default states: all low ---------- */
	    // DATA=0, CLK=0, WS=0, VALID=0
	    DATA_GPIO->BSRR = ((DATA_MASK |  EN_STREAM_PIN ) << 16);
}



// global/static bir yerde:
static uint32_t stb_state = 0; // 0: low, 1: high

static inline void stb_toggle_fast(void)
{
    if (stb_state == 0) {
    	CONT_GPIO->BSRR = CLK_PIN;          // set
    	stb_state = 1;
    } else {
    	CONT_GPIO->BSRR = (CLK_PIN << 16);  // reset
    	stb_state = 0;
    }
}

static inline void send_byte_toggle(uint8_t data)
{
    uint32_t set = (((uint32_t)data << DATA_SHIFT) & DATA_MASK);
    uint32_t rst = DATA_MASK;

    DATA_GPIO->BSRR = (rst << 16) | set;

    // küçük bir hold (DWT istemiyorsan 2-3 NOP yeterli)
    __NOP(); __NOP();

    stb_toggle_fast();

    // data hold (özellikle CAP_DLY büyütmüyorsan şart)
    __NOP(); __NOP();__NOP();
}


void send_sample24(int32_t left, int32_t right)
{
//#if FPGA_USE_READY
//    while ((DATA_GPIO->IDR & GPIO_PIN_6) == 0u) { __NOP(); }
//#endif
    left  &= 0xFFFFFF;
    right &= 0xFFFFFF;

    CONT_GPIO->BSRR = VALID_PIN;     // VALID=1
    for (volatile int i=0; i<40; i++) { __NOP(); }


    send_byte_toggle((left  >> 16) & 0xFF);
    send_byte_toggle((left  >>  8) & 0xFF);
    send_byte_toggle((left       ) & 0xFF);

    send_byte_toggle((right >> 16) & 0xFF);
    send_byte_toggle((right >>  8) & 0xFF);
    send_byte_toggle((right      ) & 0xFF);

    for (volatile int i=0; i<10; i++) { __NOP(); } // guard

	CONT_GPIO->BSRR = (VALID_PIN << 16); // VALID=0
	// küçük setup

}

volatile uint32_t g_req_timeout = 0;
volatile uint32_t g_req_fail = 0;

static inline uint32_t req_read(void)
{
    return (REQ_GPIO->IDR & REQ_PIN) ? 1u : 0u;
}

static inline void put_data8(uint8_t data)
{
    uint32_t set = (((uint32_t)data << DATA_SHIFT) & DATA_MASK);
    DATA_GPIO->BSRR = (DATA_MASK << 16) | set;
}


static inline void ack_set(void) { CONT_GPIO->BSRR = ACK_PIN; }
static inline void ack_clr(void) { CONT_GPIO->BSRR = (ACK_PIN << 16); }

// byte gönderimi (LEVEL handshake)
 static inline int send_byte_level(uint8_t b, uint32_t spin)
{
    // REQ=1 bekle
    while (!req_read()) { if (!spin--) return 0; }


    put_data8(b);
     __NOP();  // setup

     CONT_GPIO->BSRR = ACK_PIN;

    //ack_set(); // ACK=1
    // FPGA REQ=0 yapana kadar bekle
    while (req_read()) { if (!spin--) { ack_clr(); return 0; } }

    __NOP();  // hold
    CONT_GPIO->BSRR = (ACK_PIN << 16);
    //ack_clr(); // ACK=0
    return 1;
}

int send_sample24_level_syncA5(int32_t L, int32_t R)
{
    L &= 0xFFFFFF;
    R &= 0xFFFFFF;

    send_byte_level(0xA5, 500000u);
    send_byte_level((uint8_t)(L >> 16), 500000u);
    send_byte_level((uint8_t)(L >> 8), 500000u);
    send_byte_level((uint8_t)(L), 500000u);
    send_byte_level((uint8_t)(R >> 16), 500000u);
    send_byte_level((uint8_t)(R >> 8), 500000u);
    send_byte_level((uint8_t)(R), 500000u);

    return 1;
}


// ======== PINS ========
// REQ: FPGA->STM (input)  (senin READY hattın)
// STB: STM->FPGA (output) (senin STM32_CLK hattın, toggle)
// DATA: STM->FPGA 8-bit



// REQ=1 bekle (sample request)
static inline int wait_req_hi(uint32_t spin)
{
    while (!req_read()) { if (!spin--) return 0; }
    return 1;
}

// REQ=0 bekle (FPGA sample aldı demek)
static inline void wait_req_lo(uint32_t spin)
{
    while (req_read()) { if (!spin--) break; }
}

// 7 byte = A5 + L(3) + R(3)
int send_sample24_reqstb_syncA5(int32_t L, int32_t R)
{
    L &= 0xFFFFFF; R &= 0xFFFFFF;

    // FPGA request etmeden asla gönderme:
    if (!wait_req_hi(1500000u)) {
        // istersen sayaç arttır
        return 0;
    }

    uint8_t b[7] = {
        0xA5,
        (uint8_t)(L >> 16), (uint8_t)(L >> 8), (uint8_t)L,
        (uint8_t)(R >> 16), (uint8_t)(R >> 8), (uint8_t)R
    };

    for (int i=0; i<7; i++) {
        // mini kritik section (REQ gördükten sonra byte/stb sırası bozulmasın)
        uint32_t prim = __get_PRIMASK();
        __disable_irq();

        put_data8(b[i]);
        for (volatile int k=0;k<10;k++) __NOP();
        stb_toggle_fast();
        for (volatile int k=0;k<10;k++) __NOP();

        __set_PRIMASK(prim);
    }

    // FPGA REQ'yi düşürmesini bekle (opsiyonel ama önerilir)
    wait_req_lo(1500000u);

    return 1;
}


static inline uint32_t req_is_high(void)
{
    return (REQ_GPIO->IDR & REQ_PIN) ? 1u : 0u;
}


static uint32_t ack_state = 0;
static inline void ack_toggle_fast(void)
{
    if (!ack_state) { CONT_GPIO->BSRR = ACK_PIN;          ack_state = 1; }
    else            { CONT_GPIO->BSRR = (ACK_PIN << 16);  ack_state = 0; }
}


static inline void put_data8_valid(uint8_t data)
{
    uint32_t set = (((uint32_t)data << DATA_SHIFT) & DATA_MASK);
    uint32_t rst = DATA_MASK;
    DATA_GPIO->BSRR = (rst << 16) | set;
}

static inline void sync_pulse(void)
{
    // short high pulse
    CONT_GPIO->BSRR = SYNC_PIN;
    for(volatile int k=0;k<10;k++) __NOP();              // adjust if needed
    CONT_GPIO->BSRR = (SYNC_PIN << 16);
}

static inline void send_byte_toggle_valid(uint8_t data)
{
    put_data8_valid(data);
    for(volatile int k=0;k<6;k++) __NOP();                     // small data settle
    ack_toggle_fast();         // byte strobe
    for(volatile int k=0;k<6;k++) __NOP();                  // small hold
}


void fpga_set_stream_en(uint8_t en)
{
    if (en) LL_GPIO_SetOutputPin(DATA_GPIO, EN_STREAM_PIN);
    else    LL_GPIO_ResetOutputPin(DATA_GPIO, EN_STREAM_PIN);

    if (en) LL_GPIO_SetOutputPin(LED1_GPIO_Port, LED1_Pin);
    else    LL_GPIO_ResetOutputPin(LED1_GPIO_Port, LED1_Pin);
}

int send_sample24_sync_req(int32_t left, int32_t right)
{

	if (!g_audio_run) {
	        // keep lines in a benign state if needed
	        return 0;
	    }

	    // sample-level flow control: wait until FPGA requests a new sample
	    while (!req_is_high()) {
	        if (!g_audio_run) return 0;
	    }

    left  &= 0xFFFFFF;
    right &= 0xFFFFFF;

    // SYNC pulse to force framing at start of sample
    sync_pulse();

    // 6 bytes, unrolled (big endian per sample)
    send_byte_toggle_valid((uint8_t)(left  >> 16));
    send_byte_toggle_valid((uint8_t)(left  >>  8));
    send_byte_toggle_valid((uint8_t)(left));

    send_byte_toggle_valid((uint8_t)(right >> 16));
    send_byte_toggle_valid((uint8_t)(right >>  8));
    send_byte_toggle_valid((uint8_t)(right));

    return 1;
}


 static inline int32_t s24_from_uac32le_leftjust_fast(const uint8_t *p)
 {
     // little-endian CPU, 32-bit load
     int32_t v = *(const int32_t *)(const void *)p;
     return v >> 8;
 }

static inline int32_t read_s32le(const uint8_t *p)
{
    return (int32_t)((uint32_t)p[0]
        | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16)
        | ((uint32_t)p[3] << 24));
}

// bSubslotSize=4, bBitResolution=24, PCM left-justified => >>8
static inline int32_t s24_from_uac32le_leftjust(const uint8_t *p)
{
    return read_s32le(p) >> 8;   // aritmetik shift: sign korunur
}

void OnUsbIsoOut(uint8_t *usb_buf, uint32_t packetSize)
{
	const uint8_t *p = usb_buf;
	    uint32_t frames = packetSize >> 3;   // /8

	    // 2 frame/iter
	    while (frames >= 2u)
	    {
	        lr_sample_t s0, s1;

	        // frame0
	        s0.l = s24_from_uac32le_leftjust_fast(p + 0);
	        s0.r = s24_from_uac32le_leftjust_fast(p + 4);
	        p += 8;

	        // frame1
	        s1.l = s24_from_uac32le_leftjust_fast(p + 0);
	        s1.r = s24_from_uac32le_leftjust_fast(p + 4);
	        p += 8;

	        // push 2 samples (push fail olursa çık)
	        if (!audrb_push(s0)) { g_rb_drop++; return; }
	        if (!audrb_push(s1)) { g_rb_drop++; return; }

	        frames -= 2u;
	    }

	    // kalan 1 frame
	    if (frames)
	    {
	        lr_sample_t s;

	        s.l = s24_from_uac32le_leftjust_fast(p + 0);
	        s.r = s24_from_uac32le_leftjust_fast(p + 4);

	        if (!audrb_push(s)) { g_rb_drop++; return; }
	    }
}

// ---- audrb internals (audio_buffer.c içinde varsa extern ile eriş)

 extern lr_sample_t rb[AUDRB_SIZE];
 extern volatile uint32_t w, r;   // 0..RB_MASK

static inline uint32_t rb_free(uint32_t wloc, uint32_t rloc)
{
    // 1 slot boş bırak
    return (rloc - wloc - 1u) & RB_MASK;
}

// Unaligned safe load (Cortex-M7 unaligned genelde OK ama garanti olsun)
static inline uint32_t load_u32_le(const uint8_t *p)
{
    uint32_t v;
    __builtin_memcpy(&v, p, 4);   // derleyici genelde LDR'e indirger
    return v;                     // CPU little-endian -> dönüştürme yok
}

static inline uint32_t load_u32_fast(const uint8_t *p)
{
    return *(const uint32_t *)(const void *)p;   // unaligned ok
}

void OnUsbIsoOutFast(uint8_t *usb_buf, uint32_t packetSize)
{
    // Stereo 32-bit container: 8 byte / frame
    uint32_t frames = packetSize >> 3;      // /8
    if (frames == 0) return;

    // Ring snapshot
    uint32_t wloc = w;
    uint32_t rloc = r;

    uint32_t free = rb_free(wloc, rloc);
    if (free == 0) {
        g_rb_drop++;
        return;
    }

    // En fazla sığan kadar işle (overflow olmasın)
    if (frames > free) {
        frames = free;
        g_rb_drop++;  // istersen drop sayacını frames-free kadar da tutabilirsin
    }

    const uint8_t *p = usb_buf;

    // Direkt rb'ye yaz (audrb_push yok)
    while (frames >= 2u)
        {
            int32_t l0  = ((int32_t)load_u32_fast(p + 0)) >> 8;
            int32_t r0  = ((int32_t)load_u32_fast(p + 4)) >> 8;
            int32_t l1  = ((int32_t)load_u32_fast(p + 8)) >> 8;
            int32_t r1  = ((int32_t)load_u32_fast(p + 12)) >> 8;

            rb[wloc].l = l0; rb[wloc].r = r0;
            wloc = (wloc + 1u) & RB_MASK;

            rb[wloc].l = l1; rb[wloc].r = r1;
            wloc = (wloc + 1u) & RB_MASK;

            p += 16;
            frames -= 2u;
        }

        // kalan 1 frame
        if (frames)
        {
            int32_t l = ((int32_t)load_u32_fast(p + 0)) >> 8;
            int32_t rr = ((int32_t)load_u32_fast(p + 4)) >> 8;
            rb[wloc].l = l;
            rb[wloc].r = rr;
            wloc = (wloc + 1u) & RB_MASK;
        }

    // Yazılar tamam -> index publish
    __DMB();      // producer writes complete before w update
    w = wloc;
}

static inline uint8_t fpga_is_ready(void)
{
#if FPGA_USE_READY
    return (DATA_GPIO->IDR & READY_PIN) ? 1u : 0u;
#else
    return 1u;
#endif
}



#define PUMP_FRAMES_PER_CALL  1024u

void fpga_audio_pump_task_slice(void)
{
    uint32_t n = PUMP_FRAMES_PER_CALL;

    while (n-- && fpga_is_ready())
    {
        lr_sample_t s;
        if (!audrb_pop(&s)) break;     // boşsa çık
        send_sample24(s.l, s.r);
    }
}


#define LO_WATER_SAMPLES   2048u   // altına düşünce pump durur (~5.3ms @384k)
#define HI_WATER_SAMPLES   4096u   // üstüne çıkınca pump başlar
#define PUMP_BUDGET        512u    // bir turda max kaç sample basacağız
static uint8_t pump_on = 0;

static inline int fpga_ready(void)
{
    return (DATA_GPIO->IDR & GPIO_PIN_6) != 0u;
}
volatile uint32_t g_send_fail;
void fpga_audio_pump_task_budget(void)
{
	 if (!g_audio_run) {

	        pump_on = 0;
	        ack_clr();          // <<< burada
	        return;
	    }

    uint32_t fill = audrb_count();

    // Histerezis: ring dolmadan başlama
    if (!pump_on) {
        if (fill < HI_WATER_SAMPLES) return;
        pump_on = 1;
    }

    // Ring çok azaldıysa dur (USB yeni paketlerle doldursun)
    if (fill <= LO_WATER_SAMPLES) {
        pump_on = 0;
        return;
    }

    lr_sample_t s;
    uint32_t n = PUMP_BUDGET;

    while (n--) {

        if (!audrb_pop(&s)) {
            // Buraya düşmesi artık "gerçek" underrun
            g_rb_underrun++;
            pump_on = 0;
            break;
        }
        //send_sample24(s.l, s.r);
        //send_sample24_reqack (s.l, s.r);
        //send_sample24_level (s.l, s.r);
        //send_sample24_level_syncA5
        //send_sample24_sync_req

        if (!send_sample24_sync_req(s.l, s.r)) {
        	 g_send_fail++;     // veya ayrı sayaç
            pump_on = 0;
            ack_clr();      // ACK=0
            break;               // <<< çok kritik
        }

        if (audrb_count() <= LO_WATER_SAMPLES) { pump_on = 0; break; }
    }
}
