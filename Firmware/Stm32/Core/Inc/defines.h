/*
 * defines.h
 *
 *  Created on: Feb 16, 2025
 *      Author: erdal
 */

#ifndef INC_DEFINES_H_
#define INC_DEFINES_H_

#include "stm32h7xx_hal.h"
#include "Oled 256x64.h"
#include "u8g.h"


#define recive_IR_Pin IR_Pin
#define recive_IR_GPIO_Port IR_GPIO_Port

#define LCD_CS_1     LL_GPIO_SetOutputPin(GPIOE,OLED_CS_Pin);
#define LCD_CS_0     LL_GPIO_ResetOutputPin(GPIOE,OLED_CS_Pin);

#define LCD_RST_1   LL_GPIO_SetOutputPin(GPIOE,OLED_RST_Pin);
#define LCD_RST_0   LL_GPIO_ResetOutputPin(GPIOE,OLED_RST_Pin);

#define LCD_SDI_1   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
#define LCD_SDI_0   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);

#define LCD_SCL_1    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
#define LCD_SCL_0    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);

#define LCD_DC_1    LL_GPIO_SetOutputPin(GPIOE,OLED_DC_Pin);
#define LCD_DC_0    LL_GPIO_ResetOutputPin(GPIOE,OLED_DC_Pin);



#endif /* INC_DEFINES_H_ */
