/*
 * SI5340.h
 *
 *  Created on: Jun 29, 2025
 *      Author: Erdalpc
 */

#ifndef INC_SI5340_H_
#define INC_SI5340_H_
#include "stm32h7xx_hal.h"
#include "main.h"
// SPI handle tanımı
extern SPI_HandleTypeDef hspi4;

// Chip Select pin

#define SI5340_CS_LOW()      LL_GPIO_ResetOutputPin (GPIOI, CSB_SI_Pin)
#define SI5340_CS_HIGH()     LL_GPIO_SetOutputPin (GPIOI, CSB_SI_Pin)

#define SI5340_RST_LOW()      LL_GPIO_ResetOutputPin (GPIOA, RST_SI_Pin)
#define SI5340_RST_HIGH()     LL_GPIO_SetOutputPin (GPIOA, RST_SI_Pin)

// Frekans enum
typedef enum {
    SI5340_FREQ_22_5792,  // ör: 22.5792 MHz (44.1kHz katı)
    SI5340_FREQ_22_576    // ör: 22.576 MHz (48kHz katı)
} SI5340_Freq;

#define SI5340_REVD_REG_CONFIG_NUM_REGS				326

typedef struct
{
	unsigned int address; /* 16-bit register address */
	unsigned char value; /* 8-bit register data */

} si5340_revd_register_t;


// Fonksiyon prototipleri
void SI5340_WriteRegister(uint16_t regAddr, uint8_t data);
uint8_t SI5340_ReadRegister(uint16_t regAddr);
void SI5340_LoadConfig(const si5340_revd_register_t *regs, uint16_t num_regs);
void App_Init_Si5340(void);
int SampleRate_Init_Si5340(uint32_t fs);
int DSD_Clock_Init_Si5340(uint32_t dsd_rate);

/* DCO / fine trim (currently valid for the 11.2896 MHz profile exported with N1 DCO enabled) */
int SI5340_DCO_Init_N1(void);
int SI5340_DCO_FINC_N1(uint16_t steps);
int SI5340_DCO_FDEC_N1(uint16_t steps);
int SI5340_DCO_Recenter_N1(void);
int32_t SI5340_DCO_GetN1OffsetSteps(void);

#endif /* INC_SI5340_H_ */
