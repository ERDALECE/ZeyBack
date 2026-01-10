/*
 * stm32_fpga.h
 *
 *  Created on: Dec 19, 2025
 *      Author: Erdalpc
 */

#ifndef APP_STM32_FPGA_H_
#define APP_STM32_FPGA_H_
#define FPGA_USE_READY   1   // 0 yaparsan READY yok sayılır

#define BUS_CLK_EN()     __HAL_RCC_GPIOD_CLK_ENABLE()
#define DATA_GPIO        GPIOD
#define CONT_GPIO        GPIOC
#define REQ_GPIO         GPIOD
#define DATA_SHIFT       8u
#define DATA_MASK        (0xFFu << DATA_SHIFT)

#define CLK_PIN          (1u << 6)
#define EN_STREAM_PIN    (1u << 4)
#define VALID_PIN        (1u << 7)
#define READY_PIN        (1u << 6)

#define ACK_PIN          (1u << 6)
#define REQ_PIN          (1u << 6)
#define SYNC_PIN         (1u << 7)

void send_sample24(int32_t left, int32_t right);
//int send_sample24_reqack(int32_t left, int32_t right);
//int send_sample24_level(int32_t L, int32_t R);
//int send_sample24_level_syncA5(int32_t L, int32_t R);
int send_sample24_reqstb_syncA5(int32_t L, int32_t R);
int send_sample24_sync_req(int32_t left, int32_t right);

void MX_GPIO_Init_AudioPins(void);
void Audio_SetSampleRate(uint32_t fs);
void fpga_audio_pump_task(void);
void fpga_audio_pump_task_slice(void);



#endif /* APP_STM32_FPGA_H_ */
