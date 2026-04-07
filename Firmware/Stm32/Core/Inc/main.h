/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#define CSB_SI_Pin LL_GPIO_PIN_5
#define CSB_SI_GPIO_Port GPIOI
#define HDMI_RESET_Pin LL_GPIO_PIN_4
#define HDMI_RESET_GPIO_Port GPIOI
#define DSD_EN_Pin LL_GPIO_PIN_3
#define DSD_EN_GPIO_Port GPIOD
#define RSTB_Pin LL_GPIO_PIN_15
#define RSTB_GPIO_Port GPIOC
#define CS8416_CSB_Pin LL_GPIO_PIN_4
#define CS8416_CSB_GPIO_Port GPIOE
#define DSP_Pin LL_GPIO_PIN_2
#define DSP_GPIO_Port GPIOD
#define RST_SI_Pin LL_GPIO_PIN_10
#define RST_SI_GPIO_Port GPIOA
#define STM32_SYNC_Pin LL_GPIO_PIN_9
#define STM32_SYNC_GPIO_Port GPIOA
#define STM32_EN_Pin LL_GPIO_PIN_8
#define STM32_EN_GPIO_Port GPIOC
#define STM32_REQ_Pin LL_GPIO_PIN_9
#define STM32_REQ_GPIO_Port GPIOC
#define OLED_CLK_Pin LL_GPIO_PIN_12
#define OLED_CLK_GPIO_Port GPIOA
#define MUTE_Pin LL_GPIO_PIN_7
#define MUTE_GPIO_Port GPIOG
#define SEL3_Pin LL_GPIO_PIN_5
#define SEL3_GPIO_Port GPIOG
#define RCLK_ON_Pin LL_GPIO_PIN_6
#define RCLK_ON_GPIO_Port GPIOG
#define SEL2_Pin LL_GPIO_PIN_4
#define SEL2_GPIO_Port GPIOG
#define SEL1_Pin LL_GPIO_PIN_3
#define SEL1_GPIO_Port GPIOG
#define SEL0_Pin LL_GPIO_PIN_2
#define SEL0_GPIO_Port GPIOG
#define SR2_Pin LL_GPIO_PIN_2
#define SR2_GPIO_Port GPIOK
#define SR0_Pin LL_GPIO_PIN_0
#define SR0_GPIO_Port GPIOK
#define SR1_Pin LL_GPIO_PIN_1
#define SR1_GPIO_Port GPIOK
#define SR3_Pin LL_GPIO_PIN_11
#define SR3_GPIO_Port GPIOJ
#define BitRate_Pin LL_GPIO_PIN_10
#define BitRate_GPIO_Port GPIOJ
#define LED2_Pin LL_GPIO_PIN_1
#define LED2_GPIO_Port GPIOA
#define USB_RST_Pin LL_GPIO_PIN_0
#define USB_RST_GPIO_Port GPIOA
#define IR_Pin LL_GPIO_PIN_15
#define IR_GPIO_Port GPIOF
#define LED3_Pin LL_GPIO_PIN_4
#define LED3_GPIO_Port GPIOC
#define OLED_RST_Pin LL_GPIO_PIN_6
#define OLED_RST_GPIO_Port GPIOH
#define OLED_DATA_Pin LL_GPIO_PIN_15
#define OLED_DATA_GPIO_Port GPIOB
#define LED1_Pin LL_GPIO_PIN_4
#define LED1_GPIO_Port GPIOA
#define LED4_Pin LL_GPIO_PIN_5
#define LED4_GPIO_Port GPIOC
#define OLED_DC_Pin LL_GPIO_PIN_7
#define OLED_DC_GPIO_Port GPIOH
#define OLED_CS_Pin LL_GPIO_PIN_14
#define OLED_CS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
