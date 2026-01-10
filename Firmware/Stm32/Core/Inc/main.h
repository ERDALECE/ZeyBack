/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_crs.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_cortex.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_pwr.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED2_Pin LL_GPIO_PIN_2
#define LED2_GPIO_Port GPIOE
#define LED3_Pin LL_GPIO_PIN_3
#define LED3_GPIO_Port GPIOE
#define LED4_Pin LL_GPIO_PIN_4
#define LED4_GPIO_Port GPIOE
#define PDN_Pin LL_GPIO_PIN_6
#define PDN_GPIO_Port GPIOE
#define SEL1_Pin LL_GPIO_PIN_13
#define SEL1_GPIO_Port GPIOC
#define SEL0_Pin LL_GPIO_PIN_14
#define SEL0_GPIO_Port GPIOC
#define F0_Pin LL_GPIO_PIN_15
#define F0_GPIO_Port GPIOC
#define F1_Pin LL_GPIO_PIN_0
#define F1_GPIO_Port GPIOA
#define F2_Pin LL_GPIO_PIN_1
#define F2_GPIO_Port GPIOA
#define F3_Pin LL_GPIO_PIN_2
#define F3_GPIO_Port GPIOA
#define IR_Pin LL_GPIO_PIN_10
#define IR_GPIO_Port GPIOE
#define IR_EXTI_IRQn EXTI15_10_IRQn
#define OLED_CS_Pin LL_GPIO_PIN_11
#define OLED_CS_GPIO_Port GPIOE
#define OLED_CLK_Pin LL_GPIO_PIN_12
#define OLED_CLK_GPIO_Port GPIOE
#define OLED_DC_Pin LL_GPIO_PIN_13
#define OLED_DC_GPIO_Port GPIOE
#define OLED_DATA_Pin LL_GPIO_PIN_14
#define OLED_DATA_GPIO_Port GPIOE
#define OLED_RST_Pin LL_GPIO_PIN_15
#define OLED_RST_GPIO_Port GPIOE
#define VALID_Pin LL_GPIO_PIN_6
#define VALID_GPIO_Port GPIOC
#define CLK_Pin LL_GPIO_PIN_7
#define CLK_GPIO_Port GPIOC
#define SEL2_Pin LL_GPIO_PIN_8
#define SEL2_GPIO_Port GPIOC
#define CSB_SI_Pin LL_GPIO_PIN_9
#define CSB_SI_GPIO_Port GPIOC
#define RST_SI_Pin LL_GPIO_PIN_10
#define RST_SI_GPIO_Port GPIOA
#define RCLK_ON_Pin LL_GPIO_PIN_11
#define RCLK_ON_GPIO_Port GPIOA
#define HDMI_RESET_Pin LL_GPIO_PIN_12
#define HDMI_RESET_GPIO_Port GPIOA
#define HDMI_INT_Pin LL_GPIO_PIN_11
#define HDMI_INT_GPIO_Port GPIOC
#define DSP_Pin LL_GPIO_PIN_12
#define DSP_GPIO_Port GPIOC
#define CS8416_CSB_Pin LL_GPIO_PIN_1
#define CS8416_CSB_GPIO_Port GPIOD
#define RSTB_Pin LL_GPIO_PIN_7
#define RSTB_GPIO_Port GPIOD
#define BitRate_Pin LL_GPIO_PIN_4
#define BitRate_GPIO_Port GPIOB
#define USB_RST_Pin LL_GPIO_PIN_0
#define USB_RST_GPIO_Port GPIOE
#define LED1_Pin LL_GPIO_PIN_1
#define LED1_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
