/*
 * dac_gui.c
 *
 *  Created on: Dec 21, 2025
 *      Author: Erdalpc
 */


#include "main.h"
#include "i2c.h"
#include "memorymap.h"
#include "spi.h"
#include "tim.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usb_device.h"
#include "usbd_conf.h"
#include "Oled 256x64.h"
#include "u8g.h"
#include "u8g_arm.h"
#include "IRremote.h"
#include "adv7611.h"
#include "CS8416.h"
#include "SI5340.h"
#include <stdbool.h>
#include "dac_gui.h"
#include "stm32_fpga.h"

 ui_state_t ui_cur, ui_last;
 uint8_t fpga_data_changed = 0;
 uint8_t rc5_changed = 0;
 uint32_t g_ir_block_until_ms = 0;


static void apply_dsp_pin(uint8_t on)
{
    if (on) LL_GPIO_SetOutputPin(GPIOC, DSP_Pin);
    else    LL_GPIO_ResetOutputPin(GPIOC, DSP_Pin);
}

static void apply_rclk_pin(uint8_t on)
{
    if (on) LL_GPIO_SetOutputPin(GPIOA, RCLK_ON_Pin);
    else    LL_GPIO_ResetOutputPin(GPIOA, RCLK_ON_Pin);
}

 void handle_rc5(void)
{
    if (!my_decode(&results)) return;

    const uint32_t now = HAL_GetTick();
    if (now < g_ir_block_until_ms) { my_resume(); return; } // debounce

    const uint8_t ir     = (uint8_t)(results.value & 0x3F);
    const uint8_t iraddr = (uint8_t)((results.value >> 6) & 0x1F);

    if (iraddr == IR_ADDR_MAIN) {
        switch (ir) {
            case IR_KEY_DSP:
                ui_cur.dsp_on ^= 1u;
                apply_dsp_pin(ui_cur.dsp_on);
                rc5_changed = 1;
                break;

            case IR_KEY_RCLK:
                ui_cur.rclk_on ^= 1u;
                apply_rclk_pin(ui_cur.rclk_on);
                rc5_changed = 1;
                break;

            case IR_KEY_SRC:
                ui_cur.src++;
                if (ui_cur.src > 6) ui_cur.src = 0;
                rc5_changed = 1;
                break;

            default:
                break;
        }
    }
    g_ir_block_until_ms = now + 200; // soft_delay_ms(100) yerine
    my_resume();
}
static inline uint8_t bit_is_set_u32(uint32_t v, uint8_t bit)
{
    return (uint8_t)((v >> bit) & 1u);
}

static inline uint8_t read_sr_nibble(void)
{
    // IDR'leri bir kere oku (IO erişimi pahalıdır)
    const uint32_t a = GPIOA->IDR;
    const uint32_t b = GPIOB->IDR;
    const uint32_t c = GPIOC->IDR;

    uint8_t sr = 0;
    sr |= (uint8_t)(bit_is_set_u32(a, 7)  << 0);  // SR0 = PA7
    sr |= (uint8_t)(bit_is_set_u32(c, 10) << 1);  // SR1 = PC10
    sr |= (uint8_t)(bit_is_set_u32(b, 8)  << 2);  // SR2 = PB8
    sr |= (uint8_t)(bit_is_set_u32(b, 9)  << 3);  // SR3 = PB9
    return sr;
}



 void poll_fpga_data(void)
{


    // FPGA'den okunanlar (senin BR globalin var gibi)
    ui_cur.bitrate24 = (BR == 1) ? 1u : 0u;
    ui_cur.sr_code   = read_sr_nibble();

    // değişim kontrolü: tek karşılaştırma
    if (memcmp(&ui_cur, &ui_last, sizeof(ui_state_t)) != 0) {
        fpga_data_changed = 1;
        ui_last = ui_cur;
    }
}



 static const src_label_t g_src_lbl[7] = {
     { "-        XMOS",     0 },
     { "-        COAX",     5 },
     { "-        OPT1",     5 },
     { "-        OPT2",     5 },
     { "-        HDMI",     5 },
     { "-       STM32",     0 },
     { "-COAX_FPGA",        0 },
 };

 static const char* sr_code_to_pcm_rate(uint8_t code)
 {
     switch (code) {
         case 1:  return "44.1 khz";
         case 2:  return "48 khz";
         case 3:  return "88.2 khz";
         case 4:  return "96 khz";
         case 5:  return "176.4 khz";
         case 6:  return "192 khz";
         case 7:  return "352.8 khz";
         case 8:  return "384 khz";
         default: return NULL;
     }
 }

 static const char* sr_code_to_dsd(uint8_t code)
 {
     switch (code) {
         case 9:  return "DSD_64";
         case 10: return "DSD_128";
         case 11: return "DSD_256";
         case 12: return "DSD_512";
         default: return NULL;
     }
 }


  void draw_status_screen(u8g_t *u8g, const ui_state_t *s)
 {
     char line2[40];

     const uint8_t src = (s->src <= 6) ? s->src : 0;
     const src_label_t *L = &g_src_lbl[src];

     const char *pcm_rate = sr_code_to_pcm_rate(s->sr_code);
     const char *dsd_mode = sr_code_to_dsd(s->sr_code);

     if (dsd_mode) {
         // ör: "-        DSD_128"
         snprintf(line2, sizeof(line2), "-        %s", dsd_mode);
     } else if (pcm_rate) {
         // ör: "PCM - 24 bit - 192 khz"
         snprintf(line2, sizeof(line2), "PCM - %s bit - %s",
                  s->bitrate24 ? "24" : "16", pcm_rate);
     } else {
         // NO DATA / NO USB CONNECT mantığını burada sadeleştir:
         // r==0 XMOS ise "NO USB CONNECT", diğerleri "NO DATA"
         snprintf(line2, sizeof(line2),
                  (src == 0) ? "-    NO USB CONNECT" : "-      NO DATA");
     }

     u8g_FirstPage(u8g);
     do {
         // 1) Kaynak satırı
    	 u8g_SetFont(u8g, bigfont);
         u8g_DrawStr(u8g, bigfontX + L->x_ofs, bigfontY, L->name);

         // 2) DSP / RCLK küçük etiketler
         u8g_SetFont(u8g, pcmfont);
         u8g_DrawStr(u8g, dspfontX,  dspfontY,  s->dsp_on  ? "DSP"  : "-   ");
         u8g_DrawStr(u8g, rclkfontX, rclkfontY, s->rclk_on ? "RCLK" : "-    ");

         // 3) PCM/DSD/No-data satırı
         u8g_DrawStr(u8g, pcmfontX, pcmfontY, line2);

         // çerçeve
         u8g_DrawFrame(u8g, 0, 0, 256, 64);
     } while (u8g_NextPage(u8g));
 }

 static inline void gpio_write_pin(GPIO_TypeDef *port, uint32_t pin, uint8_t on)
 {
     if (on)  LL_GPIO_SetOutputPin(port, pin);
     else     LL_GPIO_ResetOutputPin(port, pin);
 }

 static inline void set_sel_lines(uint8_t sel0, uint8_t sel1, uint8_t sel2)
 {
     // SEL pinlerin GPIOC'de gibi duruyor
     gpio_write_pin(GPIOC, SEL0_Pin, sel0);
     gpio_write_pin(GPIOC, SEL1_Pin, sel1);
     gpio_write_pin(GPIOC, SEL2_Pin, sel2);
 }

 static void enter_xmos(void)
 {
     ADV7611_Reset();
 }

 static void enter_coax(void)
 {
     ADV7611_Reset();
     CS8416_SelectInput(0);
 }

 static void enter_opt1(void)
 {
     ADV7611_Reset();
     CS8416_SelectInput(1);
 }

 static void enter_opt2(void)
 {
     ADV7611_Reset();
     CS8416_SelectInput(2);
 }

 static void enter_hdmi(void)
 {
     // Senin eski kodun
     ADV7611_Init();
     ADV7611_Unmute();
     EDID_Conf();
 }

 static void enter_stm32(void)
 {
     ADV7611_Reset();
 }

 static void enter_coax_fpga(void)
 {
     ADV7611_Reset();
 }


 static const source_cfg_t g_sources[] = {
     [SRC_XMOS]      = { .sel0=0, .sel1=0, .sel2=0, .on_enter=enter_xmos },
     [SRC_COAX]      = { .sel0=1, .sel1=0, .sel2=0, .on_enter=enter_coax },
     [SRC_OPT1]      = { .sel0=1, .sel1=0, .sel2=0, .on_enter=enter_opt1 },
     [SRC_OPT2]      = { .sel0=1, .sel1=0, .sel2=0, .on_enter=enter_opt2 },
     [SRC_HDMI]      = { .sel0=0, .sel1=1, .sel2=1, .on_enter=enter_hdmi },
     [SRC_STM32]     = { .sel0=1, .sel1=0, .sel2=1, .on_enter=enter_stm32 },
     [SRC_COAX_FPGA] = { .sel0=0, .sel1=1, .sel2=0, .on_enter=enter_coax_fpga },
 };

 static uint8_t g_applied_src = 0xFF;

  void apply_input_select(uint8_t src)
 {
     if (src >= (uint8_t)(sizeof(g_sources)/sizeof(g_sources[0]))) {
         src = SRC_XMOS;
     }

     if (src == g_applied_src) return;   // değişmediyse dokunma
     g_applied_src = src;

     const source_cfg_t *cfg = &g_sources[src];

     set_sel_lines(cfg->sel0, cfg->sel1, cfg->sel2);

     if (cfg->on_enter) {
         cfg->on_enter();
     }
 }
