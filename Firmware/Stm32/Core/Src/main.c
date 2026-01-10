/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
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
#include <math.h>
#include <stdint.h>
#include "bus_dma_tim1.h"
#include "usb_iso_raw_fifo.h"
#include "audio_buffer.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
void IR_TIM3_PeriodElapsedCallback(void); // RC5/IRremote.c’den
int SampleRate_Init_Si5340(uint32_t fs);
void OnUsbIsoOut(uint8_t *usb_buf, uint32_t packetSize);

static   uint8_t first=1;
extern ui_state_t ui_cur;
extern uint8_t fpga_data_changed;
extern uint8_t rc5_changed;
uint32_t t_poll = 0;
uint32_t t_gui  = 0;
extern volatile uint32_t g_fs_pending;
extern volatile uint8_t  g_fs_apply_req;
extern volatile uint8_t  g_audio_run;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
void fpga_audio_pump_task_budget(void);
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



void IR_TIM3_PeriodElapsedCallback(void); // RC5/IRremote.c’den
extern volatile uint32_t g_audio_ticks;
volatile uint8_t rc5_pending = 0;


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_10) {   // PE10
        rc5_pending = 1;
    }
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        IR_TIM3_PeriodElapsedCallback();
        return;
    }
}

static inline void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	u8g_t u8g;
  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  SCB->CCR &= ~(1u<<3);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */
  MX_GPIO_Init();
   LL_GPIO_SetOutputPin(GPIOA, RST_SI_Pin);

   LL_GPIO_ResetOutputPin(GPIOE, USB_RST_Pin);
   LL_GPIO_ResetOutputPin(GPIOA, HDMI_RESET_Pin);

   HAL_Delay(10);
   LL_GPIO_SetOutputPin(GPIOE, USB_RST_Pin);
   LL_GPIO_SetOutputPin(GPIOA, HDMI_RESET_Pin);

   HAL_Delay(10);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_SPI4_Init();
  MX_USB_OTG_HS_PCD_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  my_enableIRIn();
    MX_USB_DEVICE_Init();

       u8g_InitComFn(&u8g, &u8g_dev_ssd1322_nhd31oled_bw_hw_spi, u8g_com_hw_spi_fn);
            SSD1322_API_set_contrast(25);
            LL_GPIO_ResetOutputPin(GPIOC, DSP_Pin);
            LL_GPIO_ResetOutputPin(GPIOC, RCLK_ON_Pin);
            LL_GPIO_ResetOutputPin(GPIOD, RSTB_Pin);
            HAL_Delay(1);
            App_Init_Si5340();

            if ( ADV7611_Init()==0){
               	  while(1);
                 }

            EDID_Conf();
            CS8416_Init();
            dwt_init();
            //CS8416_SetOutputFormat_I2S();
            MX_GPIO_Init_AudioPins();

            CONT_GPIO->BSRR = (ACK_PIN << 16);

            bus_dma_tim1_init(240000000u);
            bus_dma_tim1_set_fs(48000u);
           // bus_dma_tim1_start();
            static uint32_t t0 = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
                      while (1)
                      {


                        if ((g_audio_run==1)&&(ui_cur.src == SRC_STM32)){
                    	 // fpga_audio_pump_task_budget();
                    	  //fpga_audio_pump_task_budget();
                    	 // fpga_audio_pump_task_budget();
                        	//

                        	if (HAL_GetTick() - t0 >= 5) {   // 5ms
                        	    t0 += 5;
                        	    bus_dma_tim1_pll_task();
                        	}

                    	  if (g_fs_apply_req==1){
                    	  SampleRate_Init_Si5340(g_fs_pending);
                    	  bus_dma_tim1_set_fs(g_fs_pending);
                    	  g_fs_apply_req=0;
                    	  }
                        }else{
                        	bus_dma_tim1_stop();
                        }

                    	  uint32_t now = HAL_GetTick();
                    	     if ((int32_t)(now - t_poll) >= 100) {
                    	         t_poll = now;
                    	         poll_fpga_data();
                    	     }

                          if (fpga_data_changed || first) {
                              first = 0;
                              draw_status_screen(&u8g, &ui_cur);

                              // input selector sadece src değişince çalışsın istersen:
                              // if (ui_cur.src != ui_prev_src) { apply_input_select(ui_cur.src);
                              apply_input_select(ui_cur.src);

                            fpga_data_changed = 0;

                          }

                          if (rc5_pending) {
                            rc5_pending = 0;
                            handle_rc5();
                            rc5_changed = 0;
                          }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_4)
  {
  }
  LL_PWR_ConfigSupply(LL_PWR_LDO_SUPPLY);
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
  while (LL_PWR_IsActiveFlag_VOS() == 0)
  {
  }
  LL_RCC_HSE_EnableBypass();
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_HSE);
  LL_RCC_PLL1P_Enable();
  LL_RCC_PLL1Q_Enable();
  LL_RCC_PLL1R_Enable();
  LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_2_4);
  LL_RCC_PLL1_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
  LL_RCC_PLL1_SetM(25);
  LL_RCC_PLL1_SetN(480);
  LL_RCC_PLL1_SetP(2);
  LL_RCC_PLL1_SetQ(20);
  LL_RCC_PLL1_SetR(10);
  LL_RCC_PLL1_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL1_IsReady() != 1)
  {
  }

   /* Intermediate AHB prescaler 2 when target frequency clock is higher than 80 MHz */
   LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1)
  {

  }
  LL_RCC_SetSysPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
  LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_2);
  LL_RCC_SetAPB4Prescaler(LL_RCC_APB4_DIV_2);
  LL_SetSystemCoreClock(480000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
  LL_RCC_ConfigMCO(LL_RCC_MCO1SOURCE_PLL1QCLK, LL_RCC_MCO1_DIV_2);
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  LL_RCC_PLL2Q_Enable();
  LL_RCC_PLL2_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_2_4);
  LL_RCC_PLL2_SetVCOOutputRange(LL_RCC_PLLVCORANGE_MEDIUM);
  LL_RCC_PLL2_SetM(25);
  LL_RCC_PLL2_SetN(90);
  LL_RCC_PLL2_SetP(2);
  LL_RCC_PLL2_SetQ(2);
  LL_RCC_PLL2_SetR(2);
  LL_RCC_PLL2_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL2_IsReady() != 1)
  {
  }

  LL_RCC_PLL3P_Enable();
  LL_RCC_PLL3R_Enable();
  LL_RCC_PLL3_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_2_4);
  LL_RCC_PLL3_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
  LL_RCC_PLL3_SetM(16);
  LL_RCC_PLL3_SetN(128);
  LL_RCC_PLL3_SetP(8);
  LL_RCC_PLL3_SetQ(2);
  LL_RCC_PLL3_SetR(8);
  LL_RCC_PLL3_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL3_IsReady() != 1)
  {
  }

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER3;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_1MB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
