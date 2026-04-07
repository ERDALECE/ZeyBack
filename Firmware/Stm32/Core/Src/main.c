/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "memorymap.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"
#include "fmc.h"

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
#include <math.h>
#include <stdint.h>
#include "audio_buffer.h"
#include "bus_poll_fpga.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
void IR_TIM3_PeriodElapsedCallback(void); // RC5/IRremote.c’den
int SampleRate_Init_Si5340(uint32_t fs);
static   uint8_t first=1;
extern ui_state_t ui_cur;
extern uint8_t fpga_data_changed;
extern uint8_t rc5_changed;
uint32_t t_poll = 0;
static uint32_t t_dbg  = 0;
extern volatile uint32_t g_fs_pending;
extern volatile uint8_t  g_fs_apply_req;
extern volatile uint8_t  g_audio_run;
extern uint32_t g_fs_i2s;
static   uint8_t stm32_chg=0;
extern volatile uint32_t g_fpga_req_low_streak_max;
extern UART_HandleTypeDef huart4;
extern volatile int32_t  g_usb_fb_adj;   /* Adaptive feedback offset (usbd_audio.c) */
extern int SI5340_DCO_Init_N1(void);
extern int SI5340_DCO_FINC_N1(uint16_t steps);
extern int SI5340_DCO_FDEC_N1(uint16_t steps);
extern int SI5340_DCO_Recenter_N1(void);
extern int32_t SI5340_DCO_GetN1OffsetSteps(void);
static volatile uint8_t  g_fifo_resync_req = 0u; /* FIFO graceful resync isteği */
static volatile uint8_t  g_si5340_dco_ready = 0u;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

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

static uint8_t  prev_src = 0xFF;
static uint8_t  prev_audio_on = 0x00;

static inline bool time_due(uint32_t now, uint32_t *t_next, uint32_t period_ms)
{
    if ((int32_t)(now - *t_next) >= 0) {
        *t_next += period_ms;     // drift-free pacing
        return true;
    }
    return false;
}

static inline uint8_t si5340_profile_has_dco(uint32_t fs_or_clk)
{
    /* DCO (Digital Controlled Oscillator) destekleyen profiller.
     * Kriter: Si5340_REG_N_FSTEP_MSK != 0 (ilgili .h dosyasinda 0x0339 kaydi)
     *   44.h  → 0x1D (N1 only)   88.h  → 0x1D   176.h → 0x1D
     *   352.h → 0x1F (all N)   (352800 Hz / DSD512 profili de DCO destekler)
     * PCM base rate veya dogrudan DSD clock Hz cinsinden kabul edilir.
     */
    return (fs_or_clk == 44100u   || fs_or_clk == 2822400u  ||
            fs_or_clk == 88200u   || fs_or_clk == 5644800u  ||
            fs_or_clk == 176400u  || fs_or_clk == 11289600u ||
            fs_or_clk == 352800u  || fs_or_clk == 22579200u ||
            fs_or_clk == 705600u  || fs_or_clk == 44158400u) ? 1u : 0u;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
   MX_GPIO_Init();
   LL_GPIO_ResetOutputPin(GPIOA, USB_RST_Pin);
   HAL_Delay(100);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */
  LL_GPIO_SetOutputPin(GPIOI, RST_SI_Pin);

   LL_GPIO_ResetOutputPin(GPIOA, USB_RST_Pin);
   LL_GPIO_ResetOutputPin(GPIOI, HDMI_RESET_Pin);

   HAL_Delay(30);
   LL_GPIO_SetOutputPin(GPIOA, USB_RST_Pin);
   LL_GPIO_SetOutputPin(GPIOI, HDMI_RESET_Pin);

   HAL_Delay(30);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FMC_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_SPI4_Init();
  MX_TIM3_Init();
  MX_USB_OTG_HS_PCD_Init();
  MX_UART4_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  my_enableIRIn();
     MX_USB_DEVICE_Init();




            // LL_GPIO_ResetOutputPin(GPIOC, DSP_Pin);
            // LL_GPIO_ResetOutputPin(GPIOC, RCLK_ON_Pin);
             LL_GPIO_ResetOutputPin(GPIOC, RSTB_Pin);
             HAL_Delay(1);
             App_Init_Si5340();



             if ( ADV7611_Init()==0){
                	  while(1);
                  }



             EDID_Conf();
             CS8416_Init();

             dwt_init();
             //CS8416_SetOutputFormat_I2S();

            // bus_dma_tim1_init(240000000u);
             SampleRate_Init_Si5340(44100);
           //  bus_dma_tim1_set_fs(44100u);

             LL_GPIO_ResetOutputPin(MUTE_GPIO_Port, MUTE_Pin);
            // bus_dma_tim1_start();
             //static uint32_t t0 = 0;

             AudioBuffer_Init(AudioBuffer_Instance(), 0);  /* ring buffer sıfırla */
                bus_poll_init();
                fpga_set_stream_en(0);                        /* başlangıçta stream kapalı */
                SampleRate_Init_Si5340(44100u);
                g_si5340_dco_ready = si5340_profile_has_dco(44100u);
                if (g_si5340_dco_ready) {
                    SI5340_DCO_Init_N1();
                    SI5340_DCO_Recenter_N1();
                }
            // static uint32_t g_fs_pending_old = 44100;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
                while (1)
                                {

                          	  // debug_audio_print_once(&huart4, 2000);

                          	     uint32_t now = HAL_GetTick();

                                    /* ----------------------------------------------------
                                     * 1. Stream aktif mi?
                                     *    g_audio_run: USB ISO OUT başladığında UAC2 stack set eder.
                                     *    ui_cur.src == SRC_STM32: kullanıcı USB kaynağını seçmiş.
                                     * ---------------------------------------------------- */
                                    uint8_t audio_on = (g_audio_run && (ui_cur.src == SRC_STM32)) ? 1u : 0u;


                                    /* ----------------------------------------------------
                                     * 2. Stream durum geçişi
                                     * ---------------------------------------------------- */
                                    if (audio_on != prev_audio_on || ui_cur.src != prev_src)
                                    {
                                        prev_audio_on = audio_on;
                                        prev_src      = ui_cur.src;

                                        if (audio_on)
                                        {
                                            /* Stream başladı:
                                             *   a) MUTE: pre-fill tamamlanana kadar ses kesilir
                                             *   b) RESYNC: FPGA'daki kısmi L/R çiftini sil
                                             *   c) EN_STREAM yükselt: FPGA FIFO'yu flush eder (~640ns)
                                             *   d) FIFO ön doldurma: FPGA FIFO prog_empty (4096)
                                             *      eşiğini aşacak kadar veri birikmesini bekle.
                                             *      Beklenmezse FIFO bos baslar, her USB timing
                                             *      sapmasi FIFO'yu bosaltir → citirtilar.
                                             */
                                            LL_GPIO_SetOutputPin(MUTE_GPIO_Port, MUTE_Pin);
                                            HAL_Delay(3); /* Mute devresinin kapanmasini bekle */
                                            fpga_send_resync();
                                            fpga_set_stream_en(1);   /* AudioBuffer sifirla, s_running=1 */
                                            if (g_si5340_dco_ready) {
                                                SI5340_DCO_Recenter_N1();
                                            }

                                            /* AudioBuffer'in ~131072 byte birikmesini bekle */
                                            {
                                                uint32_t t0 = HAL_GetTick();
                                                while (AudioBuffer_LevelBytes(AudioBuffer_Instance()) < 131072u) {
                                                    if ((uint32_t)(HAL_GetTick() - t0) > 300u) break;
                                                }
                                            }
                                            /* FIFO'ya hizla ~8192 entry yaz (prog_empty esigini bol as) */
                                            bus_poll_service(8192u);
                                            HAL_Delay(5); /* FIFO cikisinin stabilize olmasini bekle */
                                            LL_GPIO_ResetOutputPin(MUTE_GPIO_Port, MUTE_Pin);
                                        }
                                        else
                                        {
                                            /* Stream durdu: once MUTE, sonra FPGA durdur
                                             * (ters sirada FPGA dururken ses yolu acik → pop) */
                                            LL_GPIO_SetOutputPin(MUTE_GPIO_Port, MUTE_Pin);
                                            HAL_Delay(3);
                                            fpga_set_stream_en(0);
                                        }

                                        apply_input_select(ui_cur.src);
                                    }

                                    /* ----------------------------------------------------
                                     * 3. Polling transfer — sadece stream aktifken
                                     *
                                     *    max_frames = 128:
                                     *      44.1kHz → 44 frame/ms → 128 frame ≈ 2.9ms
                                     *      384kHz  → 384 frame/ms → 128 frame ≈ 0.33ms
                                     *    Ana döngü her turda çağrılır; underrun olursa
                                     *    istatistikte görünür, ses kesilmez (FIFO zero-hold).
                                     * ---------------------------------------------------- */
                                    if (audio_on)
                                    {
                                        bus_poll_service(512u);
                                    }

                                    /* ----------------------------------------------------
                                     * 3b. Graceful FIFO Resync
                                     *
                                     * FPGA FIFO saat kaymasindan dolayi prog_empty'e
                                     * duserse ve USB adaptive feedback cevap vermezse,
                                     * crackling yerine ~25ms sessizlik ile FIFO yeniden
                                     * doldurulur. Akis: MUTE → FIFO flush → AudioBuffer
                                     * doldur → FIFO on-doldur → UNMUTE.
                                     *
                                     * DSD256: 131072 byte / 5.644MB/s = ~23ms doldurma
                                     * DSD64:  131072 byte / 1.411MB/s = ~93ms doldurma
                                     * ---------------------------------------------------- */
                                    if (audio_on && g_fifo_resync_req)
                                    {
                                        g_fifo_resync_req = 0u;
                                        g_usb_fb_adj      = 0;   /* adj sifirla: temiz baslangic */

                                        LL_GPIO_SetOutputPin(MUTE_GPIO_Port, MUTE_Pin); /* sus */
                                        HAL_Delay(3); /* Mute devresinin kapanmasini bekle */
                                        fpga_send_resync();
                                        if (g_si5340_dco_ready) {
                                            SI5340_DCO_Recenter_N1();
                                        }
                                        //fpga_set_stream_en(1);   /* AudioBuffer & FIFO sifirla */

                                        {
                                            uint32_t t0 = HAL_GetTick();
                                            while (AudioBuffer_LevelBytes(AudioBuffer_Instance()) < 131072u) {
                                                if ((uint32_t)(HAL_GetTick() - t0) > 400u) break;
                                            }
                                        }
                                        bus_poll_service(32768u); /* FIFO on-doldur */
                                        HAL_Delay(5); /* FIFO cikisinin stabilize olmasini bekle */
                                        LL_GPIO_ResetOutputPin(MUTE_GPIO_Port, MUTE_Pin); /* ses ac */
                                    }

                                    /* ----------------------------------------------------
                                     * 4. Sample rate değişimi
                                     *
                                     * PCM→DSD256 geçişinde iki ardışık rate değişimi olur:
                                     *   a) EP0_RxReady: g_fs_pending = DoP taşıyıcı (ör. 705600)
                                     *   b) enter_dop_mode_runtime() (USB ISR, 131072 byte bekleme
                                     *      sırasında): g_fs_pending = Si5340 frekansı (ör. 176400)
                                     *      ve aynı anda FPGA DSD moduna alınır.
                                     *
                                     * BUG (düzeltildi): Eski kodda (a) sonrası UNMUTE yapılıyor,
                                     * FPGA DSD modundayken Si5340 hâlâ yanlış frekansta → kare dalga.
                                     *
                                     * FIX: do...while(g_fs_apply_req) döngüsü — DoP tespiti
                                     * yeni bir g_fs_apply_req tetiklerse MUTE korunarak (b) de
                                     * uygulanır; UNMUTE yalnızca rate kararlı olduktan sonra yapılır.
                                     * ---------------------------------------------------- */
                                    if (audio_on && (g_fs_apply_req || stm32_chg))
                                    {
                                        LL_GPIO_SetOutputPin(MUTE_GPIO_Port, MUTE_Pin);   /* ses kes */
                                        HAL_Delay(10);

                                        do {
                                            /* --------------------------------------------------
                                             * ÖNCE temizle: SampleRate_Init_Si5340() içindeki
                                             * HAL_Delay(300) sırasında USB ISR enter_dop_mode_runtime()
                                             * çağırıp g_fs_apply_req=1 set edebilir.
                                             * Temizleme SONRAYA bırakılırsa ISR'nin set ettiği değer
                                             * hemen ezilir → do...while erken çıkar → UNMUTE yanlış
                                             * Si5340 frekansıyla yapılır → kare dalga.
                                             * Başa alınınca: ISR yeni rate'i set ederse döngü bir tur
                                             * daha döner ve g_fs_pending ile Si5340 yeniden yüklenir.
                                             * -------------------------------------------------- */
                                            g_fs_apply_req = 0u;
                                            stm32_chg      = 0u;
                                            g_usb_fb_adj   = 0;   /* Yeni sample rate, offset sifirla */

                                            AudioBuffer_Init(AudioBuffer_Instance(), 0);      /* ring sıfırla */
                                            SampleRate_Init_Si5340(g_fs_pending);             /* SI5340 (~315ms: 300ms HAL_Delay + lock) */
                                            g_si5340_dco_ready = si5340_profile_has_dco(g_fs_pending);
                                            if (g_si5340_dco_ready) {
                                                SI5340_DCO_Init_N1();
                                                SI5340_DCO_Recenter_N1();
                                            }
                                            fpga_send_resync();                               /* kısmi L/R temizle */
                                            fpga_set_stream_en(1);                            /* stream aç, s_running=1 */

                                            /* --------------------------------------------------
                                             * FIFO Ön Doldurma:
                                             * Bu bekleme sırasında USB ISR çalışır; DoP tespit
                                             * edilirse enter_dop_mode_runtime() g_fs_apply_req'u
                                             * tekrar set eder → döngü bir tur daha döner.
                                             * -------------------------------------------------- */
                                            {
                                                uint32_t t0 = HAL_GetTick();
                                                while (AudioBuffer_LevelBytes(AudioBuffer_Instance()) < 131072u) {
                                                    if ((uint32_t)(HAL_GetTick() - t0) > 300u) break;
                                                }
                                            }

                                        } while (g_fs_apply_req); /* DoP ikinci rate geçişi: MUTE korunarak uygula */

                                        /* FIFO'ya hizla ~8192 entry yaz (prog_empty eşiğini (~4096) bol aş) */
                                        bus_poll_service(32768u);

                                        HAL_Delay(15);
                                        LL_GPIO_ResetOutputPin(MUTE_GPIO_Port, MUTE_Pin); /* sesi aç */
                                    }

                                    /* ----------------------------------------------------
                                     * 5. Debug: stats + FPGA durum (2000ms)
                                     * ---------------------------------------------------- */
                                    if (g_dsd_mode != DSD_MODE_OFF && si5340_profile_has_dco(g_dsd_clock_hz)) {
                                    if (time_due(now, &t_dbg, 2000u) && audio_on)
                                    {
                                        /* Static buffer: HAL_UART_Transmit_IT async calisir,
                                         * stack frame cokmeleri onlemek icin static kullan. */
                                        static char dbg_buf[128];
                                        fpga_stats_t dbg = fpga_get_stats();
                                        uint32_t fpga_st = fpga_read_status();
                                        uint32_t ab_lvl  = AudioBuffer_LevelBytes(AudioBuffer_Instance());
                                        int32_t dco_off = SI5340_DCO_GetN1OffsetSteps();
                                        int  len = snprintf(dbg_buf, sizeof(dbg_buf),
                                            "under=%lu ovfl=%lu fr=%lu ab=%lu fpga=0x%02lX dco=%ld rs=%lu\r\n",
                                            (unsigned long)dbg.underruns,
                                            (unsigned long)dbg.overflows,
                                            (unsigned long)dbg.frames_sent,
                                            (unsigned long)ab_lvl,
                                            (unsigned long)(fpga_st & 0xFFu),
                                            (long)dco_off,
                                            (unsigned long)dbg.resync_count);
                                        HAL_UART_Transmit(&huart4, (uint8_t*)dbg_buf, (uint16_t)len, 10u);
                                        fpga_reset_stats();
                                    }

                                    /* ----------------------------------------------------
                                     * 6. FPGA durum sorgusu + adaptive feedback (200ms)
                                     * ---------------------------------------------------- */

                                    if (time_due(now, &t_poll, 20u))
                                    {
                                        poll_fpga_data();

                                        if (audio_on)
                                        {
                                            #define AB_TARGET        131072u
                                            #define AB_HIGH1         163840u
                                            #define AB_LOW1           98304u
                                            #define AB_LOW2           65536u
                                            #define AB_LOW3           32768u
                                            #define DCO_MAX_STEPS        780
                                            #define RESYNC_CNT            80   /* 80 x 20ms = 1.6 s */

                                            static uint16_t s_low_cnt = 0;
                                            uint32_t ab_lvl = AudioBuffer_LevelBytes(AudioBuffer_Instance());
                                            int32_t  dco_off = SI5340_DCO_GetN1OffsetSteps();

                                            g_usb_fb_adj = 0;   /* MPD feedback artik devre disi */

                                            if (ab_lvl < AB_LOW3)
                                            {
                                                /* cok kritik: hizli yavaslat */
                                                if (dco_off > -DCO_MAX_STEPS) SI5340_DCO_FDEC_N1(10);
                                                s_low_cnt++;
                                            }
                                            else if (ab_lvl < AB_LOW2)
                                            {
                                                if (dco_off > -DCO_MAX_STEPS) SI5340_DCO_FDEC_N1(5);
                                                s_low_cnt++;
                                            }
                                            else if (ab_lvl < AB_LOW1)
                                            {
                                                if (dco_off > -DCO_MAX_STEPS) SI5340_DCO_FDEC_N1(2);
                                                s_low_cnt = 0;
                                            }
                                            else if (ab_lvl > AB_HIGH1)
                                            {
                                                /* fazla doluyorsa biraz hizlandir */
                                                if (dco_off < DCO_MAX_STEPS) SI5340_DCO_FINC_N1(2);
                                                s_low_cnt = 0;
                                            }
                                            else
                                            {
                                                /* orta bolgede yavasca merkeze don */
                                                if (dco_off > 0) {
                                                    SI5340_DCO_FDEC_N1(1);
                                                } else if (dco_off < 0) {
                                                    SI5340_DCO_FINC_N1(1);
                                                }
                                                s_low_cnt = 0;
                                            }

                                            /* sadece uzun sure cok dusukse resync */
                                            if (s_low_cnt >= RESYNC_CNT) {
                                                g_fifo_resync_req = 0u;
                                                s_low_cnt = 0u;
                                            }
                                        }
                                        else
                                        {
                                            g_usb_fb_adj = 0;
                                        }
                                    }
                                   }
                                    /* ----------------------------------------------------
                                     * 7. FPGA I2S kaynak değişimi (ADV7611/CS8416)
                                     * ---------------------------------------------------- */
                                    if (fpga_data_changed || first)
                                    {
                                        first             = 0;
                                        fpga_data_changed = 0;

                                        if (ui_cur.src == SRC_I2S_FPGA) {
                                            SampleRate_Init_Si5340(g_fs_i2s);
                                            stm32_chg = 1u;
                                        }
                                    }

                                    /* ----------------------------------------------------
                                     * 8. IR remote
                                     * ---------------------------------------------------- */
                                    handle_rc5();
                                    rc5_changed = 0;

                                } /* while(1) */

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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
  LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_4_8);
  LL_RCC_PLL1_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
  LL_RCC_PLL1_SetM(12);
  LL_RCC_PLL1_SetN(240);
  LL_RCC_PLL1_SetP(2);
  LL_RCC_PLL1_SetQ(20);
  LL_RCC_PLL1_SetR(2);
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
  LL_RCC_PLL3Q_Enable();
  LL_RCC_PLL3_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_1_2);
  LL_RCC_PLL3_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
  LL_RCC_PLL3_SetM(32);
  LL_RCC_PLL3_SetN(129);
  LL_RCC_PLL3_SetP(2);
  LL_RCC_PLL3_SetQ(2);
  LL_RCC_PLL3_SetR(2);
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

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER5;
  MPU_InitStruct.BaseAddress = 0x60000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_2MB;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

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
