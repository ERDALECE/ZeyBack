/*
  
  u8g_arm.c
  

  u8g utility procedures for LPC11xx

  Universal 8bit Graphics Library
  
  Copyright (c) 2013, olikraus@gmail.com
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

 
  The following delay procedures must be implemented for u8glib. This is done in this file:

  void u8g_Delay(uint16_t val)		Delay by "val" milliseconds
  void u8g_MicroDelay(void)		Delay be one microsecond
  void u8g_10MicroDelay(void)	Delay by 10 microseconds
  
  Additional requirements:
  
*/

#include "u8g_arm.h"
#include "defines.h"
GPIO_InitTypeDef GPIO_InitStructure;
extern SPI_HandleTypeDef hspi4;
#define SET_CONTRAST_CURRENT       0xC1
#define MASTER_CONTRAST_CURRENT    0xC7

//////////////////////////////////////////////////////////////////////////////////////////////
// ARM Cortex-M3 islemciler icin kapali dongu bekleme fonksiyonu
// 32mhz hse optimize-1 ( 10->13us ) (1->2us)
//////////////////////////////////////////////////////////////////////////////////////////////
void delay_micro_seconds(uint32_t us) 
{
	us*=4;
	while(us--)
	{
		__NOP();
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////
// ARM Cortex-M3 islemciler icin girilen deger kadar X us bekleyen fonksiyon
//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////
// ARM Cortex-M3 islemciler icin girilen deger kadar Ms bekleyen fonksiyon
//////////////////////////////////////////////////////////////////////////////////////////////
void u8g_Delay(uint16_t val)
{
  delay_micro_seconds(1000UL*(uint32_t)val);
}
//////////////////////////////////////////////////////////////////////////////////////////////
// ARM Cortex-M3 islemciler icin sabit 1 us bekleyen fonksiyon
//////////////////////////////////////////////////////////////////////////////////////////////
void u8g_MicroDelay(void)
{
  delay_micro_seconds(1);
}
//////////////////////////////////////////////////////////////////////////////////////////////
// ARM Cortex-M3 islemciler icin sabit 10 us bekleyen fonksiyon
//////////////////////////////////////////////////////////////////////////////////////////////
void u8g_10MicroDelay(void)
{
  delay_micro_seconds(10);
}
//////////////////////////////////////////////////////////////////////////////////////////////
// U8Glib pin kriptolamay1 yapan fonksiyon (pin bilgisi 1 byte a saklanir)
//////////////////////////////////////////////////////////////////////////////////////////////
/* convert "port" and "bitpos" to internal pin number */
/*
uint8_t u8g_Pin(uint8_t port, uint8_t bitpos)
{
  port <<= 4;// arm * 16
  port += bitpos;
  return port;
}
*/
//////////////////////////////////////////////////////////////////////////////////////////////
// ARM Cortex-M3 islemciler icin girilen pini OUTPUT yapan fonksiyon
//////////////////////////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////////////////////////
// ARM Cortex-M3 islemciler icin u8glib API haberlesme yapisi
//////////////////////////////////////////////////////////////////////////////////////////////
uint8_t u8g_com_sw_spi_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr)
{
 switch(msg)
  {
    case U8G_COM_MSG_STOP:
      break;
    
    case U8G_COM_MSG_INIT:
			Oled_Init();
      break;
    
    case U8G_COM_MSG_ADDRESS:   /* define cmd (arg_val = 0) or data mode (arg_val = 1) */
		{
			if( arg_val == 0 ){ 
						LCD_DC_0;
			}
			else {
						LCD_DC_1;
			}
			
     break;
		}
    case U8G_COM_MSG_CHIP_SELECT:
		{
      if ( arg_val == 0 )
			{
					LCD_CS_0		
			}      
			else
			{
					LCD_CS_1
			}				
			
      break;
		}
    case U8G_COM_MSG_RESET:
		{
			if( arg_val == 1 )
					LCD_RST_1
			else 				
					LCD_RST_0
     
      break;
		}
    case U8G_COM_MSG_WRITE_BYTE:
		{
			
			Write_Datau(arg_val);
      break;
		}
    case U8G_COM_MSG_WRITE_SEQ:
		{
        register uint8_t *ptr = arg_ptr;
        while( arg_val > 0 )
        {
					
					Write_Datau(*ptr++);		
							
          arg_val--;
        }	
			break;				
		}
    case U8G_COM_MSG_WRITE_SEQ_P:
		{
        register uint8_t *ptr = arg_ptr;
        while( arg_val > 0 )
        {
					
					Write_Datau(*ptr++);		
					                   			
          arg_val--;
        }		
      break;				
		}			
  }
  return 1;
}


void SSD1322_HW_SPI_send_byte(uint8_t byte_to_transmit)
{
	HAL_SPI_Transmit(&hspi4, &byte_to_transmit, 1, 10);
}

void SSD1322_HW_SPI_send_array(uint8_t *array_to_transmit, uint32_t array_size)
{
	HAL_SPI_Transmit(&hspi4, array_to_transmit, array_size, 100);
}

void SSD1322_HW_msDelay(uint32_t milliseconds)
{
	HAL_Delay(milliseconds);
}

void SSD1322_data(uint8_t data)
{
	LCD_CS_0;
	//SSD1322_HW_drive_DC_high();
	SSD1322_HW_SPI_send_byte(data);

	LCD_CS_1;
}

void SSD1322_API_command(uint8_t command)
{
	LCD_CS_0;
	LCD_DC_0;
	SSD1322_HW_SPI_send_byte(command);
	LCD_CS_1;
}

//====================== data ========================//
/**
 *  @brief Sends data byte to SSD1322
 */
void SSD1322_API_data(uint8_t data)
{
	LCD_CS_0;
	LCD_DC_1;
	SSD1322_HW_SPI_send_byte(data);
	LCD_CS_1;
}

void Clear_API_ram(void)
{ unsigned char x,y;
SSD1322_API_command(0x15);
SSD1322_API_data(0x00);
SSD1322_API_data(0x77);
SSD1322_API_command(0x75);
	SSD1322_API_data(0x00);
	SSD1322_API_data(0x7f);
	SSD1322_API_command(0x5C);
	for(y=0;y<128;y++)
		{ for(x=0;x<120;x++)
			{   SSD1322_API_data(0x00);
			}
		}
}

//====================== contrast ========================//
/**
 *  @brief Sets contrast between brightest and darkest pixels.
 */
void SSD1322_API_set_contrast(uint8_t contrast)
{
	SSD1322_API_command(SET_CONTRAST_CURRENT);
	SSD1322_API_data(contrast);
}

//====================== brightness ========================//
/**
 *  @brief Should set brightness, but actual effect is similar to setting contrast.
 */
void SSD1322_API_set_brightness(uint8_t brightness)
{
	SSD1322_API_command(MASTER_CONTRAST_CURRENT);
	SSD1322_API_data(0x0F & brightness);            //first 4 bits have to be 0
}


void SSD1322_API_init()
{
	LCD_RST_0  //Reset pin low
	SSD1322_HW_msDelay(1);                  //1ms delay
	LCD_RST_1 //Reset pin high
	SSD1322_HW_msDelay(50);                 //50ms delay
	SSD1322_API_command(0xFD);     //set Command unlock
	SSD1322_API_data(0x12);
	SSD1322_API_command(0xAE);     //set display off
	SSD1322_API_command(0xB3);     //set display clock divide ratio
	SSD1322_API_data(0x91);
	SSD1322_API_command(0xCA);     //set multiplex ratio
	SSD1322_API_data(0x3F);
	SSD1322_API_command(0xA2);   //set display offset to 0
	SSD1322_API_data(0x00);
	SSD1322_API_command(0xA1);   //start display start line to 0
	SSD1322_API_data(0x00);
	SSD1322_API_command(0xA0);   //set remap and dual COM Line Mode
	SSD1322_API_data(0x14);
	SSD1322_API_data(0x11);
	SSD1322_API_command(0xB5);   //disable IO input
	SSD1322_API_data(0x00);
	SSD1322_API_command(0xAB);   //function select
	SSD1322_API_data(0x01);
	SSD1322_API_command(0xB4);   //enable VSL extern
	SSD1322_API_data(0xA0);
	SSD1322_API_data(0xFD);
	SSD1322_API_command(0xC1);   //set contrast current
	SSD1322_API_data(0xFF);
	SSD1322_API_command(0xC7);   //set master contrast current
	SSD1322_API_data(0x0F);
	SSD1322_API_command(0xB9);   //default grayscale
	SSD1322_API_command(0xB1);   //set phase length
	SSD1322_API_data(0xE2);
	SSD1322_API_command(0xD1);   //enhance driving scheme capability
	SSD1322_API_data(0x82);
	SSD1322_API_data(0x20);
	SSD1322_API_command(0xBB);   //first pre charge voltage
	SSD1322_API_data(0x1F);
	SSD1322_API_command(0xB6);   //second pre charge voltage
	SSD1322_API_data(0x08);
	SSD1322_API_command(0xBE);   //VCOMH
	SSD1322_API_data(0x07);
	SSD1322_API_command(0xA6);   //set normal display mode
	SSD1322_API_command(0xA9);   //no partial mode
	SSD1322_HW_msDelay(10);              //stabilize VDD
	SSD1322_API_command(0xA6);
	Clear_API_ram();
	SSD1322_API_command(0xAF);   //display on
	SSD1322_API_command(0xA6);
	SSD1322_HW_msDelay(50);               //stabilize VDD
}

uint8_t u8g_com_hw_spi_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr)
{
 switch(msg)
  {
    case U8G_COM_MSG_STOP:
      break;

    case U8G_COM_MSG_INIT:
    	SSD1322_API_init();
      break;

    case U8G_COM_MSG_ADDRESS:   /* define cmd (arg_val = 0) or data mode (arg_val = 1) */
		{
			if( arg_val == 0 ){
						LCD_DC_0;
			}
			else {
						LCD_DC_1;
			}

     break;
		}
    case U8G_COM_MSG_CHIP_SELECT:
		{
      if ( arg_val == 0 )
			{
					LCD_CS_0
			}
			else
			{
					LCD_CS_1
			}

      break;
		}
    case U8G_COM_MSG_RESET:
		{
			if( arg_val == 1 )
					LCD_RST_1
			else
					LCD_RST_0

      break;
		}
    case U8G_COM_MSG_WRITE_BYTE:
		{

			SSD1322_data(arg_val);
      break;
		}
    case U8G_COM_MSG_WRITE_SEQ:
		{
        register uint8_t *ptr = arg_ptr;
        while( arg_val > 0 )
        {

        	SSD1322_data(*ptr++);

          arg_val--;
        }

        break;
		}
    case U8G_COM_MSG_WRITE_SEQ_P:
		{
        register uint8_t *ptr = arg_ptr;
        while( arg_val > 0 )
        {

        	SSD1322_data(*ptr++);
        	//SSD1322_data(u8g_pgm_read(ptr));
        	//ptr++;
          arg_val--;
        }
      break;
		}
  }
  return 1;
}





