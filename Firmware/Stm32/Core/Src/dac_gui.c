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
#include "IRremote.h"
#include "adv7611.h"
#include "CS8416.h"
#include "SI5340.h"
#include <stdbool.h>
#include "dac_gui.h"


 ui_state_t ui_cur, ui_last;
 uint8_t fpga_data_changed = 0;
 uint8_t rc5_changed = 0;
 uint32_t g_ir_block_until_ms = 0;

 uint32_t g_fs_i2s =44100;


static void apply_dsp_pin(uint8_t on)
{
    if (on) LL_GPIO_SetOutputPin(GPIOD, DSP_Pin);
    else    LL_GPIO_ResetOutputPin(GPIOD, DSP_Pin);
}

static void apply_rclk_pin(uint8_t on)
{
    if (on) LL_GPIO_SetOutputPin(GPIOG, RCLK_ON_Pin);
    else    LL_GPIO_ResetOutputPin(GPIOG, RCLK_ON_Pin);
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

            case IR_KEY_SRC_P:
                ui_cur.src++;
                if (ui_cur.src > 9) ui_cur.src = 0;
                rc5_changed = 1;
                break;

            case IR_KEY_SRC_M:
                 //ui_cur.src--;
                 ui_cur.src = (ui_cur.src == 0) ? 9 : (ui_cur.src - 1);
                rc5_changed = 1;
                break;

            default:
                break;
        }
    }
    g_ir_block_until_ms = now + 250; // soft_delay_ms(100) yerine
    my_resume();
}
static inline uint8_t bit_is_set_u32(uint32_t v, uint8_t bit)
{
    return (uint8_t)((v >> bit) & 1u);
}

static inline uint8_t read_sr_nibble(void)
{
    // IDR'leri bir kere oku (IO erişimi pahalıdır)
    const uint32_t k = GPIOK->IDR;
    const uint32_t j = GPIOJ->IDR;


    uint8_t sr = 0;
    sr |= (uint8_t)(bit_is_set_u32(k, 0)  << 0);  // SR0 = PK0
    sr |= (uint8_t)(bit_is_set_u32(k, 1) << 1);  // SR1 = PK1
    sr |= (uint8_t)(bit_is_set_u32(k, 2)  << 2);  // SR2 = PK2
    sr |= (uint8_t)(bit_is_set_u32(j, 11)  << 3);  // SR3 = PJ11
    return sr;
}



 void poll_fpga_data(void)
{


    // FPGA'den okunanlar
    ui_cur.bitrate24 = (BR == 1) ? 1u : 0u;
    ui_cur.sr_code   = read_sr_nibble();

    // değişim kontrolü: tek karşılaştırma
    if (memcmp(&ui_cur, &ui_last, sizeof(ui_state_t)) != 0) {
        fpga_data_changed = 1;
        ui_last = ui_cur;
    }
}


 static inline void gpio_write_pin(GPIO_TypeDef *port, uint32_t pin, uint8_t on)
 {
     if (on)  LL_GPIO_SetOutputPin(port, pin);
     else     LL_GPIO_ResetOutputPin(port, pin);
 }

 static inline void set_sel_lines(uint8_t sel0, uint8_t sel1, uint8_t sel2, uint8_t sel3)
 {

     gpio_write_pin(GPIOG, SEL0_Pin, sel0);
     gpio_write_pin(GPIOG, SEL1_Pin, sel1);
     gpio_write_pin(GPIOG, SEL2_Pin, sel2);
     gpio_write_pin(GPIOG, SEL3_Pin, sel3);
 }

 static void enter_xmos(void)
 {
     ADV7611_Reset();
     LL_GPIO_ResetOutputPin(GPIOA,LED1_Pin);
 }

 static void enter_coax(void)
 {
     ADV7611_Reset();
     CS8416_SelectInput(0);
     LL_GPIO_ResetOutputPin(GPIOA,LED1_Pin);
 }

 static void enter_opt1(void)
 {
     ADV7611_Reset();
     CS8416_SelectInput(1);
     LL_GPIO_ResetOutputPin(GPIOA,LED1_Pin);
 }

 static void enter_opt2(void)
 {
     ADV7611_Reset();
     CS8416_SelectInput(2);
     LL_GPIO_ResetOutputPin(GPIOA,LED1_Pin);
 }

 static void enter_hdmi(void)
 {
     // Senin eski kodun
     ADV7611_Init();
     ADV7611_Unmute();
     EDID_Conf();
     LL_GPIO_ResetOutputPin(GPIOA,LED1_Pin);
 }

 static void enter_stm32(void)
 {
     ADV7611_Reset();
     LL_GPIO_ResetOutputPin(GPIOC,LED1_Pin);
 }

 static void enter_coax_fpga(void)
 {
     ADV7611_Reset();
     LL_GPIO_ResetOutputPin(GPIOA,LED1_Pin);
 }

 static void enter_I2S_FPGA(void)
  {
      ADV7611_Reset();
      LL_GPIO_ResetOutputPin(GPIOA,LED1_Pin);
  }

 static void enter_OPT1_FPGA(void)
  {
      ADV7611_Reset();
      LL_GPIO_ResetOutputPin(GPIOA,LED1_Pin);
  }

 static void enter_OPT2_FPGA(void)
   {
       ADV7611_Reset();
       LL_GPIO_ResetOutputPin(GPIOA,LED1_Pin);
   }

 static const source_cfg_t g_sources[] = {
     [SRC_STM32]     = { .sel0=0, .sel1=0, .sel2=0, .sel3=0, .on_enter=enter_stm32 },
     [SRC_COAX]      = { .sel0=1, .sel1=0, .sel2=0, .sel3=0, .on_enter=enter_coax },
     [SRC_OPT1]      = { .sel0=0, .sel1=1, .sel2=0, .sel3=0, .on_enter=enter_opt1 },
     [SRC_OPT2]      = { .sel0=1, .sel1=1, .sel2=0, .sel3=0, .on_enter=enter_opt2 },
     [SRC_HDMI]      = { .sel0=0, .sel1=0, .sel2=1, .sel3=0, .on_enter=enter_hdmi },
     [SRC_XMOS]      = { .sel0=1, .sel1=0, .sel2=1, .sel3=0, .on_enter=enter_xmos },
     [SRC_COAX_FPGA] = { .sel0=0, .sel1=1, .sel2=1, .sel3=0, .on_enter=enter_coax_fpga },
	 [SRC_OPT1_FPGA] = { .sel0=1, .sel1=1, .sel2=1, .sel3=0, .on_enter=enter_OPT1_FPGA },
	 [SRC_OPT2_FPGA] = { .sel0=0, .sel1=0, .sel2=0, .sel3=1, .on_enter=enter_OPT2_FPGA },
	 [SRC_I2S_FPGA]  = { .sel0=1, .sel1=0, .sel2=0, .sel3=1, .on_enter=enter_I2S_FPGA },
 };

 static uint8_t g_applied_src = 0xFF;

  void apply_input_select(uint8_t src)
 {
     if (src >= (uint8_t)(sizeof(g_sources)/sizeof(g_sources[0]))) {
         src = SRC_STM32;
     }else if(src <= 0){
    	 src = SRC_STM32;
     }

     if (src == g_applied_src) return;   // değişmediyse dokunma
     g_applied_src = src;

     const source_cfg_t *cfg = &g_sources[src];

     set_sel_lines(cfg->sel0, cfg->sel1, cfg->sel2, cfg->sel3);

     if (cfg->on_enter) {
         cfg->on_enter();
     }
 }
