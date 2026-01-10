/*
 * CS8416.h
 *
 *  Created on: Jun 29, 2025
 *      Author: Erdalpc
 */

#ifndef INC_CS8416_H_
#define INC_CS8416_H_
#include "main.h"
#include "stm32h7xx_hal.h"

// SPI handle tanımı
extern SPI_HandleTypeDef hspi2;

// CS8416 Chip Select pin kontrolü (örnek)


#define CS8416_CS_LOW()       LL_GPIO_ResetOutputPin (GPIOD, CS8416_CSB_Pin)
#define CS8416_CS_HIGH()      LL_GPIO_SetOutputPin (GPIOD, CS8416_CSB_Pin)

// Fonksiyon prototipleri
void CS8416_WriteRegister(uint8_t regAddr, uint8_t data);
HAL_StatusTypeDef CS8416_ReadRegister(uint8_t regAddr, uint8_t *data);
void CS8416_SelectInput(uint8_t input);
void CS8416_Init(void);


#endif /* INC_CS8416_H_ */
