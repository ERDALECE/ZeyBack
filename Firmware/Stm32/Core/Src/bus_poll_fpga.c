/*
 * bus_poll_fpga.c
 *
 * STM32H750 → Artix-7 FMC 32-bit polling audio transfer
 * FPGA entity: fmc32_to_i2s_dsd (PCM + Native DSD + DoP support)
 */

#include "bus_poll_fpga.h"
#include "audio_buffer.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_ll_gpio.h"
#include <string.h>

extern UART_HandleTypeDef huart4;

/* Sample format conversion */
static inline uint32_t s32_to_fmc(uint32_t s32_le)
{
    return s32_le;  /* Left-justified, direct pass-through */
}

/* Wrap-safe 32-bit ring buffer read */
static inline uint32_t rd_u32_wrap(const uint8_t *mem, uint32_t cap, uint32_t rp)
{
    if (rp <= (cap - 4u)) {
        return *(const uint32_t *)(const void *)&mem[rp];
    }
    /* Wrap boundary: byte by byte */
    uint32_t b0 = mem[rp];
    uint32_t b1 = mem[(rp + 1u) >= cap ? (rp + 1u) - cap : rp + 1u];
    uint32_t b2 = mem[(rp + 2u) >= cap ? (rp + 2u) - cap : rp + 2u];
    uint32_t b3 = mem[(rp + 3u) >= cap ? (rp + 3u) - cap : rp + 3u];
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

/* Module internal state */
static volatile uint8_t s_running  = 0u;
static volatile uint8_t s_dsd_mode = 0u;

/*
 * DoP (DSD over PCM) mode flag.
 *
 * DoP formatinda her 32-bit sample soyle yapilandirilmistir (little-endian USB):
 *   Byte[0] = DSD[7:0]    → uint32 bit [7:0]
 *   Byte[1] = DSD[15:8]   → uint32 bit [15:8]   ← DSD[15] = FPGA'ya ilk cikan bit
 *   Byte[2] = DoP marker (0x05 veya 0xFA) → bit [23:16]  ← SOYULACAK
 *   Byte[3] = 0x00 (sign ext) → bit [31:24]
 *
 * DoP DSD bit sirasi:
 *   Her 16-bit DSD payload icinde bit15 en eski (ilk zamanda) DSD bitidir.
 *   FPGA bit31'i once cikariyor (MSB-first). Bu nedenle:
 *   - Frame1 16-bit DSD → 32-bit kelimenin bit[31:16] bolumune
 *   - Frame2 16-bit DSD → 32-bit kelimenin bit[15:0]  bolumune
 *   Bu sekilde bit31 (= frame1 bit15 = en eski DSD bit) once cikar.
 *
 * DoP FPGA besleme hizi:
 *   DSD64: BCLK = 2822400 Hz, FIFO pop = BCLK/32 = 88200/sn
 *   USB frame hizi = 176400/sn → 2 USB frame = 1 FPGA yazimi (32-bit DSD)
 *   Her bus_poll_service dongu adiminda 2 USB frame tuketilir, 1 FPGA yazimi yapilir.
 *
 * s_dop_mode = 1: Byte[2] maskelenir, 2 frame birlestirilip 32-bit DSD gider.
 * s_dop_mode = 0: 32-bit ham data gider (native DSD veya PCM).
 *
 * Native DSD (Interface 2): s_dsd_mode=1, s_dop_mode=0 → 32 ham DSD bit
 * DoP        (Interface 1): s_dsd_mode=1, s_dop_mode=1 → 2×16-bit DoP → 32-bit DSD
 * PCM        (Interface 1): s_dsd_mode=0, s_dop_mode=0 → 32-bit PCM
 */
static volatile uint8_t s_dop_mode = 0u;

static fpga_stats_t s_stats = {0};

/* Globals */
volatile uint8_t  g_audio_run = 0u;
extern volatile uint32_t g_fs_pending;
extern volatile uint8_t  g_fs_apply_req;
extern volatile uint32_t g_usb_fb_16_16;

/* DSD mode globals (from usbd_audio.c) */
extern volatile uint8_t  g_dsd_mode;
extern volatile uint32_t g_dsd_clock_hz;
extern volatile uint8_t  g_dop_mode;

/*============================================================================
 * Public API
 *============================================================================*/

void bus_poll_init(void)
{
    s_running  = 0u;
    s_dsd_mode = 0u;
    s_dop_mode = 0u;
    memset(&s_stats, 0, sizeof(s_stats));
}

void bus_poll_start(void) { s_running = 1u; }
void bus_poll_stop(void)  { s_running = 0u; }

/*
 * fpga_set_dsd_mode
 *
 * FPGA'nin DSD/PCM mod pinini set eder.
 * bus_poll_service() icindeki transfer yolunu da belirler.
 *
 * ONEMLI: bus_poll_init()'ten SONRA cagrılmalidir.
 * bus_poll_init() s_dsd_mode'u sifirlar; onceden cagrilirsa ezilir.
 */
void fpga_set_dsd_mode(uint8_t dsd_on)
{
    s_dsd_mode = dsd_on;

    if (dsd_on) {
        LL_GPIO_SetOutputPin(DSD_MODE_GPIO, DSD_MODE_PIN);
    } else {
        LL_GPIO_ResetOutputPin(DSD_MODE_GPIO, DSD_MODE_PIN);
        s_dop_mode = 0u;  /* DSD kapaninca DoP de kapanir */
    }
}

/*
 * fpga_set_dop_mode
 *
 * DoP marker stripping ve 2-frame packing'i etkinlestirir/devre disi birakar.
 * DoP aktifken bus_poll_service() her iki ardisik USB frame'i birlestirir:
 *   FPGA word[31:16] = frame1 DSD[15:0]   (frame1 bit15 = ilk DSD biti)
 *   FPGA word[15:0]  = frame2 DSD[15:0]
 * Boylece 176400 Hz USB hizi → 88200 Hz FPGA besleme hizina dusurulur.
 *
 * Cagri sirasi (usbd_audio.c icinden):
 *   bus_poll_init()      ← s_dop_mode = 0
 *   fpga_set_dop_mode(1) ← s_dop_mode = 1 (DoP aktif)
 *   fpga_set_dsd_mode(1) ← s_dsd_mode = 1
 */
void fpga_set_dop_mode(uint8_t dop_on)
{
    s_dop_mode = dop_on;
}

/*
 * fpga_set_stream_en
 */
void fpga_set_stream_en(uint8_t en)
{
    if (en) {
        AudioBuffer *ab = AudioBuffer_Instance();
        ab->size   = 0u;
        ab->rd_ptr = 0u;
        ab->wr_ptr = 0u;
        ab->state  = AB_OK;

        LL_GPIO_SetOutputPin(EN_STREAM_GPIO, EN_STREAM_PIN);
        LL_GPIO_SetOutputPin(LED1_GPIO_Port, LED1_Pin);
        s_running = 1u;
    } else {
        LL_GPIO_ResetOutputPin(EN_STREAM_GPIO, EN_STREAM_PIN);
        LL_GPIO_ResetOutputPin(LED1_GPIO_Port, LED1_Pin);
        s_running = 0u;
    }
}

uint32_t     fpga_read_status(void)  { return FPGA_CTRL_REG; }
uint8_t      fpga_is_dsd_mode(void)  { return s_dsd_mode; }
uint8_t      fpga_is_dop_mode(void)  { return s_dop_mode; }
fpga_stats_t fpga_get_stats(void)    { return s_stats; }
void         fpga_reset_stats(void)  { memset(&s_stats, 0, sizeof(s_stats)); }

/*
 * fpga_send_resync
 */
void fpga_send_resync(void)
{
    FPGA_CTRL_REG = 0x00000000u;
    __DSB();
    s_stats.resync_count++;
}

/*============================================================================
 * bus_poll_service - Main transfer loop
 *
 * PCM, DoP ve Native DSD transferini yonetir.
 *
 * Mod tablosu:
 *   s_dsd_mode=0, s_dop_mode=0 → PCM: format donusumu ile 32-bit gonder
 *   s_dsd_mode=1, s_dop_mode=0 → Native DSD: ham 32-bit gonder (Interface 2)
 *   s_dsd_mode=1, s_dop_mode=1 → DoP: 2 USB frame → 1 adet 32-bit DSD gonder
 *
 * DoP packing detayi (FPGA MSB-first, bit31 once):
 *   USB Frame 1 (S32_LE): [0x00][marker][DSD_H][DSD_L]
 *     → 16-bit DSD: (l_raw1 & 0x0000FFFF) = {DSD[15:8], DSD[7:0]}
 *     → DSD[15] = ilk DSD biti (en eski)
 *   USB Frame 2 (S32_LE): ayni format, sonraki 16 DSD biti
 *
 *   FPGA 32-bit DSD kelimesi:
 *     bit[31:16] = frame1 & 0xFFFF   (bit31 = DSD bit ilk = dogru siralama)
 *     bit[15:0]  = frame2 & 0xFFFF
 *
 *   FPGA besleme hizi: DSD64 icin 176400/2 = 88200 Hz = BCLK(2822400)/32 ✓
 *
 * FMC yazi sirasi: L once, R sonra
 *   L yazildiktan sonra FPGA have_l <= '1' set eder
 *   R yazilinca FPGA stb_rx <= '1' (FIFO push)
 *============================================================================*/
void bus_poll_service(uint32_t max_frames)
{
    if (!s_running) return;

    AudioBuffer *ab  = AudioBuffer_Instance();
    const uint32_t cap = ab->capacity;

    /* Overflow recovery:
     * wr_ptr sifirlanmaz! DMA hala eski wr_ptr'a yaziyor olabilir.
     * rd_ptr'i wr_ptr'a esitleyerek eski/gecersiz veriyi atlariz. */
    if (ab->state == AB_OVFL) {
        ab->rd_ptr = ab->wr_ptr;
        ab->size   = 0u;
        ab->state  = AB_OK;
        s_stats.overflows++;
        fpga_send_resync();
        return;
    }

    /* Yerel kopyalar: volatile okuma sayisini azalt */
    const uint8_t dsd = s_dsd_mode;
    const uint8_t dop = s_dop_mode;

    /*
     * DoP modunda her dongu adimi 2 USB frame (16 byte) tuketir, 1 FPGA yazimi yapar.
     * PCM / Native DSD modunda 1 USB frame (8 byte) tuketir.
     */
    const uint32_t min_bytes = (dsd && dop) ? 16u : 8u;

    for (uint32_t i = 0u; i < max_frames; i++)
    {
        /* Minimum buffer kontrolu */
        if (ab->size < min_bytes) {
            s_stats.underruns++;
            break;
        }

        uint32_t rp = ab->rd_ptr;

        /* Frame 1 oku */
        uint32_t l_raw = rd_u32_wrap(ab->mem, cap, rp);
        rp += 4u; if (rp >= cap) rp -= cap;

        uint32_t r_raw = rd_u32_wrap(ab->mem, cap, rp);
        rp += 4u; if (rp >= cap) rp -= cap;

        ab->rd_ptr = rp;
        /* Atomik azaltma: USB ISR (AudioBuffer_Receive) ayni anda ab->size'i
         * artirabilir. __disable_irq() tight loop icinde 352800x/sn USB ISR'i
         * bloke edip SOF kayiplarına yol actigindan LDREX/STREX kullanilir.
         * ISR araya girerse STREX basarisiz olur, dongu yeniden dener — IRQ
         * gecikmesi sifir, atomiklik garantili (Cortex-M7 ARMv7-M). */
        {
            uint32_t old_sz, new_sz;
            do {
                old_sz = __LDREXW(&ab->size);
                new_sz = old_sz - 8u;
            } while (__STREXW(new_sz, &ab->size));
        }
        ab->state  = AB_OK;

        /*-----------------------------------------------------------------
         * Yazilimsal PROG_FULL beklemesi (L yaziminden once, tek seferlik).
         *
         * NWAIT donanimi FMC'de etkinlestirilmisse bu okuma zaten NWAIT
         * tarafindan stall'lanir ve ayni islevi gorur (gecikme = 0).
         * NWAIT etkin degilse yazilim bu dongu ile akis kontrolunu saglar;
         * FIFO tasmasi ve AudioBuffer asiri bosalmasi onlenir.
         *-----------------------------------------------------------------*/
        while (FPGA_CTRL_REG & FPGA_ST_PROG_FULL) { /* FIFO bos yer bekle */ }

        if (!dsd) {
            /*-----------------------------------------------------------------
             * PCM modu: format donusumu ile gonder
             *-----------------------------------------------------------------*/
            FPGA_DATA_REG = s32_to_fmc(l_raw);
            __DSB();
            FPGA_DATA_REG = s32_to_fmc(r_raw);
            __DSB();
        }
        else if (dop) {
            /*-----------------------------------------------------------------
             * DoP modu: 2 USB frame → 1 adet 32-bit DSD FPGA yazimi
             *
             * S32_LE DoP/MPD format (her 32-bit frame, little-endian):
             *   Byte[3] = bits[31:24] = DoP marker (0x05/0xFA)  ← ATLANIR
             *   Byte[2] = bits[23:16] = DSD[15:8]  (eski bitler, ilk zamanda)
             *   Byte[1] = bits[15:8]  = DSD[7:0]   (yeni bitler)
             *   Byte[0] = bits[7:0]   = 0x00        (padding)
             *
             * Dogru extraction: (l_raw >> 8) & 0x0000FFFF
             *   = {DSD[15:8], DSD[7:0]} = tam 16-bit DSD
             *   bit15 = DSD[15] = en eski DSD biti (ilk zamanda) ✓
             *
             *   YANLIS: l_raw & 0x0000FFFF = {DSD[7:0], 0x00} = eksik + sifir
             *           → duty cycle ~%25 → DC offset (osiloskopda goruldu)
             *
             * FPGA 32-bit DSD kelimesi:
             *   bit[31:16] = frame1 extract  → FPGA bit31 = DSD[15] once cikar ✓
             *   bit[15:0]  = frame2 extract
             *
             * DSD64: BCLK=2822400 Hz → FIFO pop = 2822400/32 = 88200/sn
             *        2 frame/yazim → 176400/2 = 88200 FPGA yazimi/sn ✓
             *-----------------------------------------------------------------*/

            /* Frame 2 oku (min_bytes=16 kontrolu gecildi, veri garanti) */
            uint32_t l_raw2 = rd_u32_wrap(ab->mem, cap, rp);
            rp += 4u; if (rp >= cap) rp -= cap;
            uint32_t r_raw2 = rd_u32_wrap(ab->mem, cap, rp);
            rp += 4u; if (rp >= cap) rp -= cap;

            ab->rd_ptr = rp;
            {
                uint32_t old_sz, new_sz;
                do {
                    old_sz = __LDREXW(&ab->size);
                    new_sz = old_sz - 8u;
                } while (__STREXW(new_sz, &ab->size));
            }

            /* 16-bit DSD extraction: marker(bits31:24) atlanir, DSD bits23:8 alinir */
            FPGA_DATA_REG = (((l_raw  >> 8) & 0x0000FFFFu) << 16) | ((l_raw2 >> 8) & 0x0000FFFFu);
            __DSB();
            FPGA_DATA_REG = (((r_raw  >> 8) & 0x0000FFFFu) << 16) | ((r_raw2 >> 8) & 0x0000FFFFu);
            __DSB();
        }
        else {
            /*-----------------------------------------------------------------
             * Native DSD modu (Interface 2): ham 32-bit DSD gonder
             *-----------------------------------------------------------------*/
            FPGA_DATA_REG = l_raw;
            __DSB();
            FPGA_DATA_REG = r_raw;
            __DSB();
        }

        s_stats.frames_sent++;
    }
}
