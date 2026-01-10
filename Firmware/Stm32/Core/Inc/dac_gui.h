/*
 * dac_gui.h
 *
 *  Created on: Dec 21, 2025
 *      Author: Erdalpc
 */

#ifndef INC_DAC_GUI_H_
#define INC_DAC_GUI_H_

#include "u8g.h"


#define  bigfont United20//Azonix18
#define  pcmfont United10//Digital10
#define  bigfontX 10
#define  bigfontY 32
#define  pcmfontX 40
#define  pcmfontnodataX 64
#define  pcmfontdsdX 64
#define  pcmfontY 55
#define  dspfontX 5
#define  dspfontY 20
#define  rclkfontX 200
#define  rclkfontY 20


#define IR_ADDR_MAIN   16
#define IR_KEY_DSP     5
#define IR_KEY_RCLK    6
#define IR_KEY_SRC     15

#define BR         (LL_GPIO_IsInputPinSet(GPIOB, BitRate_Pin))
#define DSP_1     LL_GPIO_SetOutputPin(GPIOC, DSP1_Pin);
#define DSP_0     LL_GPIO_ResetOutputPin(GPIOC, DSP1_Pin);
#define RCLK_1     LL_GPIO_SetOutputPin(GPIOA, RCLK_ON_Pin);
#define RCLK_0     LL_GPIO_ResetOutputPin(GPIOA, RCLK_ON_Pin);


typedef struct {
    uint8_t  src;        // r
    uint8_t  sr_code;    // last_samplerate (senin kodundaki değerler)
    uint8_t  bitrate24;  // 1: 24bit, 0: 16bit (senin bitrate==1 mantığın)
    uint8_t  dsp_on;     // dsp_filt
    uint8_t  rclk_on;    // rclk
} ui_state_t;


typedef struct {
    const char *name;
    uint8_t x_ofs; // küçük hizalama farklarını da tabloda tut
} src_label_t;

typedef void (*enter_fn_t)(void);

typedef struct {
    uint8_t sel0, sel1, sel2;
    enter_fn_t on_enter;
} source_cfg_t;

enum {
    SRC_XMOS = 0,
    SRC_COAX = 1,
    SRC_OPT1 = 2,
    SRC_OPT2 = 3,
    SRC_HDMI = 4,
    SRC_STM32 = 5,
    SRC_COAX_FPGA = 6,
};


void poll_fpga_data(void);
void apply_input_select(uint8_t src);
void draw_status_screen(u8g_t *u8g, const ui_state_t *s);
void handle_rc5(void);

#endif /* INC_DAC_GUI_H_ */
