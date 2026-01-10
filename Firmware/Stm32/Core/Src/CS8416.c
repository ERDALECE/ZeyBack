/*
 * CS8416.c
 *
 *  Created on: Jun 29, 2025
 *      Author: Erdalpc
 */

#include "CS8416.h"

// CS8416 register write
void CS8416_WriteRegister(uint8_t regAddr, uint8_t data)
{
    uint8_t txData[3];
    txData[0] = 0x20;
    txData[1] = regAddr; // MSB=0 for write
    txData[2] = data;

    CS8416_CS_LOW();

   HAL_SPI_Transmit(&hspi2, txData, 3, HAL_MAX_DELAY);

    CS8416_CS_HIGH();
    HAL_Delay(1);
    CS8416_CS_HIGH();
}

// CS8416 register read
HAL_StatusTypeDef CS8416_ReadRegister(uint8_t regAddr, uint8_t *data)
{
	 HAL_StatusTypeDef status;
	    uint8_t txData[2];

	    // 1. MAP set
	    txData[0] = 0x20;  // chip addr + W
	    txData[1] = regAddr;

	    CS8416_CS_LOW();
	    status = HAL_SPI_Transmit(&hspi2, txData, 2, HAL_MAX_DELAY);
	    CS8416_CS_HIGH();
	    if (status != HAL_OK) return status;

	    // 2. Read başlat
	    uint8_t readAddr = 0x21;
	    CS8416_CS_LOW();
	    status = HAL_SPI_Transmit(&hspi2, &readAddr, 1, HAL_MAX_DELAY);
	    if (status != HAL_OK) {
	        CS8416_CS_HIGH();
	        return status;
	    }

	    // 3. Data oku
	    uint8_t dummy = 0xFF;
	    status = HAL_SPI_TransmitReceive(&hspi2, &dummy, data, 1, HAL_MAX_DELAY);
	    CS8416_CS_HIGH();
	    HAL_Delay(1);
	    return status;
}

void CS8416_SelectInput(uint8_t input)
{

	uint8_t reg;
	reg= 0x80 + (input * 0x08 );
    CS8416_WriteRegister(0x04, reg);
    CS8416_CS_HIGH();

}


void CS8416_Init(void)
{

     uint8_t reg;
	LL_GPIO_ResetOutputPin(GPIOD, RSTB_Pin);
	 CS8416_CS_HIGH();
	HAL_Delay(1);
	LL_GPIO_SetOutputPin(GPIOD, RSTB_Pin);
	 CS8416_CS_LOW();
	HAL_Delay(1);
	 CS8416_CS_HIGH();

    CS8416_WriteRegister(0x05, 0x85);
    CS8416_WriteRegister(0x00, 0x0C);
    CS8416_WriteRegister(0x01, 0x00);
    CS8416_WriteRegister(0x02, 0x49);
    CS8416_WriteRegister(0x03, 0x30);
    CS8416_WriteRegister(0x04, 0x80);
    CS8416_WriteRegister(0x06, 0x7F);
    CS8416_WriteRegister(0x07, 0x7F);


    CS8416_ReadRegister(0x0A, &reg);

}
