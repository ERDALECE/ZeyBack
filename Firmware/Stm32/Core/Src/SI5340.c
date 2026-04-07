/*
 * SI5340.c
 *
 * SI5340 Clock Generator Driver with PCM and DSD support
 */

#include "SI5340.h"
#include "SI5340_conf.h"
#include "Si5340-RevD-r2rdac-Registers-44.h"
#include "Si5340-RevD-r2rdac-Registers-48.h"
#include "Si5340-RevD-r2rdac-Registers-88.h"
#include "Si5340-RevD-r2rdac-Registers-96.h"
#include "Si5340-RevD-r2rdac-Registers-176.h"
#include "Si5340-RevD-r2rdac-Registers-192.h"
#include "Si5340-RevD-r2rdac-Registers-352.h"
#include "Si5340-RevD-r2rdac-Registers-384.h"
#include "Si5340-RevD-r2rdac-Registers-708.h"
#include "Si5340-RevD-r2rdac-Registers-768.h"

// DSD clock config dosyaları (mevcut config'ler kullanılacak)
// DSD64  = 2.8224 MHz = 44.1kHz * 64  -> si5340_bclk_2_8224 (44.1k config)
// DSD128 = 5.6448 MHz = 88.2kHz * 64  -> si5340_bclk_5_6448 (88.2k config)
// DSD256 = 11.2896 MHz = 176.4kHz * 64 -> si5340_bclk_11_2896 (176.4k config)  
// DSD512 = 22.5792 MHz = 352.8kHz * 64 -> si5340_bclk_22_5792 (352.8k config)

#include <stdint.h>


/* DCO / fine trim support
 * Current exported DCO-enabled profile is the 11.2896 MHz OUT0 config (N1).
 * FINC/FDEC are self-clearing control bits.
 */
#define SI5340_REG_FINC_FDEC      0x001D
#define SI5340_REG_N1_UPDATE      0x0317
#define SI5340_REG_N_FSTEP_MSK    0x0339
#define SI5340_N1_ONLY_MASK       0x1D   /* only N1 responds to FINC/FDEC */

static volatile int32_t s_si5340_n1_offset_steps = 0;

static uint32_t fs_old = 0;
static uint8_t current_mode = 0;  // 0 = PCM, 1 = DSD

// Write register
void SI5340_SPI_WriteRegister(uint8_t regAddr, uint8_t data)
{
    uint8_t tx[4];

    tx[0] = 0x00;        // Set Address command
    tx[1] = regAddr;     // Address LSB
    tx[2] = 0x40;        // Write command
    tx[3] = data;        // Data

    SI5340_CS_LOW();
    HAL_SPI_Transmit(&hspi4, tx, 4, HAL_MAX_DELAY);
    SI5340_CS_HIGH();
}

void SI5340_SPI_Write16(uint16_t regAddr, uint8_t data)
{
    uint8_t page = (regAddr >> 8) & 0xFF;
    uint8_t offset = regAddr & 0xFF;

    // PAGE select
    SI5340_SPI_WriteRegister(0x01, page);
    // Then offset write
    SI5340_SPI_WriteRegister(offset, data);
}

// Read register
uint8_t SI5340_SPI_ReadRegister(uint8_t regAddr)
{
    uint8_t setAddrOpcode[3] = {0x00, regAddr, 0x80};
    uint8_t data = 0;

    SI5340_CS_LOW();
    HAL_SPI_Transmit(&hspi4, setAddrOpcode, 3, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi4, &data, 1, HAL_MAX_DELAY);
    SI5340_CS_HIGH();

    return data;
}

uint8_t SI5340_SPI_Read16(uint16_t regAddr)
{
    uint8_t page = (regAddr >> 8) & 0xFF;
    uint8_t offset = regAddr & 0xFF;

    // PAGE select
    SI5340_SPI_WriteRegister(0x01, page);
    // Offset read
    return SI5340_SPI_ReadRegister(offset);
}


void SI5340_WriteRegister(uint16_t regAddr, uint8_t data)
{
    SI5340_SPI_Write16(regAddr, data);
}

uint8_t SI5340_ReadRegister(uint16_t regAddr)
{
    return SI5340_SPI_Read16(regAddr);
}

int SI5340_DCO_Init_N1(void)
{
    SI5340_WriteRegister(SI5340_REG_N_FSTEP_MSK, SI5340_N1_ONLY_MASK);
    return 0;
}

int SI5340_DCO_FINC_N1(uint16_t steps)
{
    while (steps--) {
        SI5340_WriteRegister(SI5340_REG_FINC_FDEC, 0x01); /* FINC */
        s_si5340_n1_offset_steps++;
    }
    return 0;
}

int SI5340_DCO_FDEC_N1(uint16_t steps)
{
    while (steps--) {
        SI5340_WriteRegister(SI5340_REG_FINC_FDEC, 0x02); /* FDEC */
        s_si5340_n1_offset_steps--;
    }
    return 0;
}

int SI5340_DCO_Recenter_N1(void)
{
    SI5340_WriteRegister(SI5340_REG_N1_UPDATE, 0x01); /* reload nominal N1 */
    s_si5340_n1_offset_steps = 0;
    return 0;
}

int32_t SI5340_DCO_GetN1OffsetSteps(void)
{
    return s_si5340_n1_offset_steps;
}

void SI5340_LoadConfig(const si5340_revd_register_t *regs, uint16_t num_regs)
{
    for (uint16_t i = 0; i < num_regs; i++) {
        SI5340_SPI_Write16(regs[i].address, regs[i].value);
        if (i == 6) {
            HAL_Delay(300);  // SiLabs script recommendation
        }
    }

    SI5340_SPI_Write16(0x001C, 0x01);
    SI5340_SPI_Read16(0x000C);
}

void App_Init_Si5340(void)
{
    SI5340_RST_LOW();
    HAL_Delay(1);
    SI5340_RST_HIGH();
    HAL_Delay(20);
    SI5340_CS_HIGH();

    SI5340_LoadConfig(si5340_bclk_2_8224, SI5340_REVD_REG_CONFIG_NUM_REGS);
    HAL_Delay(10);

    if ((SI5340_SPI_Read16(0x0002)) != 0x040) {
        while(1);
    }
    HAL_Delay(1);
}

/*
 * PCM Sample Rate to SI5340 config mapping
 * BCLK = Fs * 64
 */
static int si5340_config_for_pcm_fs(uint32_t fs)
{

	switch (fs) {
    case 44100:   SI5340_LoadConfig(si5340_bclk_2_8224,  SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    case 48000:   SI5340_LoadConfig(si5340_bclk_3_0720,  SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    case 88200:   SI5340_LoadConfig(si5340_bclk_5_6448,  SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    case 96000:   SI5340_LoadConfig(si5340_bclk_6_1440,  SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    case 176400:  SI5340_LoadConfig(si5340_bclk_11_2896, SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    case 192000:  SI5340_LoadConfig(si5340_bclk_12_2880, SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    case 352800:  SI5340_LoadConfig(si5340_bclk_22_5792, SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    case 384000:  SI5340_LoadConfig(si5340_bclk_24_5760, SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    case 768000:  SI5340_LoadConfig(si5340_bclk_49_1520, SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    default:
        return -1;
    }
}

/*
 * DSD Clock mapping
 * 
 * DSD64:  DSD_CLK = 2.8224 MHz  (44.1kHz base * 64)
 * DSD128: DSD_CLK = 5.6448 MHz  (88.2kHz base * 64)
 * DSD256: DSD_CLK = 11.2896 MHz (176.4kHz base * 64)
 * DSD512: DSD_CLK = 22.5792 MHz (352.8kHz base * 64)
 *
 * Note: These are the same as PCM BCLK values for 44.1/88.2/176.4/352.8 kHz!
 * The SI5340 configs we already have can be reused.
 */
static int si5340_config_for_dsd_clock(uint32_t dsd_clock_hz)
{
    switch (dsd_clock_hz) {
    case 2822400:   // DSD64
        SI5340_LoadConfig(si5340_bclk_2_8224, SI5340_REVD_REG_CONFIG_NUM_REGS);
        return 0;
    case 5644800:   // DSD128
        SI5340_LoadConfig(si5340_bclk_5_6448, SI5340_REVD_REG_CONFIG_NUM_REGS);
        return 0;
    case 11289600:  // DSD256
        SI5340_LoadConfig(si5340_bclk_11_2896, SI5340_REVD_REG_CONFIG_NUM_REGS);
        return 0;
    case 22579200:  // DSD512
        SI5340_LoadConfig(si5340_bclk_22_5792, SI5340_REVD_REG_CONFIG_NUM_REGS);
        return 0;
    case 45158400:  // DSD512
        SI5340_LoadConfig(si5340_bclk_45_1584, SI5340_REVD_REG_CONFIG_NUM_REGS);
        return 0;
    default:
        return -1;
    }
}

static void si5340_wait_lock_fallback(void)
{
    HAL_Delay(15);
}

/*
 * Initialize SI5340 for PCM sample rate
 */
int SampleRate_Init_Si5340(uint32_t fs)
{
    if (fs == 0) return -1;
    
    // DSD clock olarak gelirse (MHz cinsinden)
    if (fs >= 2000000) {
        // Bu bir DSD clock değeri
        if (fs == fs_old && current_mode == 1) return 0;
        
        SI5340_CS_HIGH();
        if (si5340_config_for_dsd_clock(fs) != 0) {
            return -1;
        }
        si5340_wait_lock_fallback();
        //SI5340_DCO_Recenter_N1();
        fs_old = fs;
        current_mode = 1;  // DSD mode
        return 0;
    }
    
    // PCM sample rate
    if (fs == fs_old && current_mode == 0) return 0;

    SI5340_CS_HIGH();
    if (si5340_config_for_pcm_fs(fs) != 0) {
        return -1;
    }
    si5340_wait_lock_fallback();
    SI5340_DCO_Recenter_N1();
    fs_old = fs;
    current_mode = 0;  // PCM mode
    return 0;
}

/*
 * Set DSD clock directly (alternative API)
 */
int DSD_Clock_Init_Si5340(uint32_t dsd_rate)
{
    uint32_t dsd_clock;
    
    switch (dsd_rate) {
    case 64:   dsd_clock = 2822400;  break;  // DSD64
    case 128:  dsd_clock = 5644800;  break;  // DSD128
    case 256:  dsd_clock = 11289600; break;  // DSD256
    case 512:  dsd_clock = 22579200; break;  // DSD512
    default:   return -1;
    }
    
    return SampleRate_Init_Si5340(dsd_clock);
}
