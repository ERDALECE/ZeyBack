/*
  
  u8g_arm.h
  
  Header file for u8g LPC11xx utility procedures 

  Universal 8bit Graphics Library
  
  Copyright (c) 2012, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  

*/


#ifndef _U8G_ARM_H
#define _U8G_ARM_H





/////////////////////////////////////////////////////////
#define STM32F4   // STM32L serisi icin bu sat1r1 kaldir
/////////////////////////////////////////////////////////


/* system helper procedures */
#define PIN(base,bit) ((base)*16+(bit))
#define DEFAULT_KEY PIN(0,1)
 
//#include <stdint.h>
#include "u8g.h"
#include "main.h"
uint8_t u8g_Pin(uint8_t port, uint8_t bitpos);
void u8g_SetPinOutput(uint8_t internal_pin_number);
void u8g_SetPinInput(uint8_t internal_pin_number);
void u8g_SetPinLevel(uint8_t internal_pin_number, uint8_t level);
uint16_t u8g_GetPinLevel(uint8_t internal_pin_number);
void SSD1322_API_set_contrast(uint8_t contrast);
void SSD1322_API_set_brightness(uint8_t brightness);
uint8_t u8g_com_sw_spi_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr);
uint8_t u8g_com_hw_spi_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr);
void Init_SPI1(void);

#endif


