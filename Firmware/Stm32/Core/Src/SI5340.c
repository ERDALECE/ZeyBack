/*
 * SI5340.c
 *
 *  Created on: Jun 29, 2025
 *      Author: Erdalpc
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

#include <stdint.h>

static uint32_t fs_old = 0;


// Write register
void SI5340_SPI_WriteRegister(uint8_t regAddr, uint8_t data)
{

	 uint8_t tx[4];

	    tx[0] = 0x00;        // Set Address command
	    tx[1] = regAddr;     // Address LSB
	    tx[2] = 0x40;        // Write command
	    tx[3] = data;        // Data

	    SI5340_CS_LOW();

	    HAL_SPI_Transmit(&hspi2, tx, 4, HAL_MAX_DELAY);

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
	   // uint8_t readDataOpcode = 0x80;
	    uint8_t data = 0;

	    // === Set Address ===
	    SI5340_CS_LOW();
	    HAL_SPI_Transmit(&hspi2, setAddrOpcode, 3, HAL_MAX_DELAY);
	   // HAL_SPI_Transmit(&hspi2, &regAddr, 1, HAL_MAX_DELAY);
	   // SI5340_CS_HIGH();

	    // === Read Data ===
	   // SI5340_CS_LOW();
	   // HAL_SPI_Transmit(&hspi2, &readDataOpcode, 1, HAL_MAX_DELAY);
	    HAL_SPI_Receive(&hspi2, &data, 1, HAL_MAX_DELAY);
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



 void SI5340_LoadConfig(const si5340_revd_register_t *regs, uint16_t num_regs)
{

    for (uint16_t i = 0; i < num_regs; i++) {
    	SI5340_SPI_Write16(regs[i].address, regs[i].value);
        if (i == 6)
          {
             HAL_Delay(300);  // SiLabs script önerisi
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

	SI5340_LoadConfig(si5340_bclk_2_8224,  SI5340_REVD_REG_CONFIG_NUM_REGS);
	//SI5340_LoadConfig(si5340_revd_registers_98_90, SI5340_REVD_REG_CONFIG_NUM_REGS);

	//SI5340_LoadConfig(si5340_revd_registers_90, SI5340_REVD_REG_CONFIG_NUM_REGS);
	HAL_Delay(10);


	if ((SI5340_SPI_Read16(0x0002))!=0x040){
			while(1);
		}
   	HAL_Delay(1);
}


static int si5340_config_for_fs(uint32_t fs)
{
    switch (fs) {
    case 44100:   SI5340_LoadConfig(si5340_bclk_2_8224,  SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    case 48000:   SI5340_LoadConfig(si5340_bclk_3_0720,  SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    case 88200:   SI5340_LoadConfig(si5340_bclk_5_6448,  SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    case 96000:   SI5340_LoadConfig(si5340_bclk_6_1440,  SI5340_REVD_REG_CONFIG_NUM_REGS);  return 0;
    case 176400:  SI5340_LoadConfig(si5340_bclk_11_2896, SI5340_REVD_REG_CONFIG_NUM_REGS); return 0;
    case 192000:  SI5340_LoadConfig(si5340_bclk_12_2880, SI5340_REVD_REG_CONFIG_NUM_REGS); return 0;
    case 352800:  SI5340_LoadConfig(si5340_bclk_22_5792, SI5340_REVD_REG_CONFIG_NUM_REGS); return 0;
    case 384000:  SI5340_LoadConfig(si5340_bclk_24_5760, SI5340_REVD_REG_CONFIG_NUM_REGS); return 0;
    default:
        return -1;
    }
}

// TODO: SI5340 lock status okumayı ekleyebilirsen en iyisi
static void si5340_wait_lock_fallback(void)
{
    HAL_Delay(15); // pratik başlangıç: 10–20ms
}

int SampleRate_Init_Si5340(uint32_t fs)
{
    if (fs == 0) return -1;
    if (fs == fs_old) return 0;

    // 1) MUTE ON (FPGA pin +/veya röle)
    // FPGA_MUTE_ON();
    // RELAY_MUTE_ON();

    // 2) FPGA fifo flush / tx reset tetikle (isteğe bağlı ama çok faydalı)
    // FPGA_AUDIO_HOLD_ON();
    // FPGA_FIFO_FLUSH_PULSE();

    // 3) SI5340 config yükle
    // (Reset şart değil; şimdilik koyacaksan da sadece ilk init'te yap)
    // SI5340_RST_LOW(); HAL_Delay(1); SI5340_RST_HIGH(); HAL_Delay(20);

    SI5340_CS_HIGH();

    if (si5340_config_for_fs(fs) != 0) {
        // RELAY_MUTE_ON(); (zaten ON)
        return -1;
    }

    // 4) Lock bekle
    si5340_wait_lock_fallback();

    // 5) FPGA hold bırak + FIFO bir miktar dolsun (çok iyi sonuç verir)
    // HAL_Delay(2);
    // FPGA_AUDIO_HOLD_OFF();

    // 6) MUTE OFF (tercihen FIFO dolduktan sonra)
    // RELAY_MUTE_OFF();
    // FPGA_MUTE_OFF();

    fs_old = fs;
    return 0;
}

