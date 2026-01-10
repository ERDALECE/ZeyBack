#include "audio_buffer.h"
#include "stm32h7xx.h"   // __DMB()
#include "usbd_audio.h"
#include <string.h>




__attribute__((section(".RAM_D2"), aligned(32)))
uint8_t usb_rx_buf[USB_HS_MAX_PACKET_SIZE];

// SPSC ring: producer = USB ISO OUT callback, consumer = main loop feeder
//__attribute__((section(".RAM_D2"), aligned(32)))
__attribute__((section(".DTCMRAM"), aligned(32)))
 lr_sample_t rb[AUDRB_SIZE];

// write/read index (mask’li)
 volatile uint32_t w = 0;
 volatile uint32_t r = 0;

// debug
volatile uint32_t g_rb_drop = 0;
volatile uint32_t g_rb_highwater = 0;

volatile uint32_t g_rb_underrun = 0;
volatile uint32_t g_rb_overrun  = 0;   // g_rb_drop yerine bunu kullanabilirsin
volatile uint32_t g_rb_lowwater = 0xFFFFFFFFu;
extern volatile uint8_t  g_audio_run;

void audrb_init(void)
{
    w = 0;
    r = 0;
    g_rb_drop = 0;
    g_rb_highwater = 0;

    g_rb_underrun = 0;
    g_rb_overrun  = 0;
    g_rb_lowwater = 0xFFFFFFFFu;
}

uint32_t audrb_count(void)
{
    // SPSC: w ve r atomic 32-bit; fark mask ile
    return (w - r) & RB_MASK;
}

uint32_t audrb_free(void)
{
    // 1 slot boş bırakıyoruz (full/empty ayırmak için)
    return (RB_MASK - audrb_count());
}

bool audrb_push(lr_sample_t s)
{
    uint32_t w_local = w;
    uint32_t r_local = r;                 // snapshot
    uint32_t next    = (w_local + 1u) & RB_MASK;

    if (next == r_local) {
        g_rb_overrun++;
        g_rb_drop++;
        return false;
    }

    rb[w_local] = s;
    __DMB();                              // rb yazısı tamam
    w = next;                             // publish

    // high-water (audrb_count çağırma yok!)
    uint32_t c = (next - r_local) & RB_MASK;
    if (c > g_rb_highwater) g_rb_highwater = c;

    return true;
}

bool audrb_pop(lr_sample_t* s)
{
	uint32_t r_local = r;
	    uint32_t w_local = w;
	    if (r_local == w_local) return false;

	    __DMB();           // producer'ın rb yazıları görünür olsun
	    *s = rb[r_local];

	    r = (r_local + 1u) & RB_MASK;
	    return true;
}

uint32_t audrb_level_samples(void)
{
    // level = şu an kaç LR sample var
    return audrb_count();
}

uint32_t audrb_capacity_samples(void)
{
    // 1 slot boş bırakıldığı için efektif kapasite AUDRB_SIZE-1
    return RB_MASK; // = AUDRB_SIZE - 1
}


