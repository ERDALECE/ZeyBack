/*
 * adv7611.c
 *
 *  Created on: Apr 14, 2025
 *      Author: erdal
 */

 #include "main.h"
 #include "adv7611.h"
 #include "adv7611_config.h"
 #include "192khz.h"

 extern I2C_HandleTypeDef hi2c1;




 void adv7611_load_edid(void)
 {
     // --- EDID Belleğini Yükle (I2C addr: 0x6C) ---
     for (uint8_t i = 0; i < 256; i += 16)
     {
         HAL_I2C_Mem_Write(&hi2c1,
                           0x6C,             // ADV7611 EDID I2C address
                           i,                // memory offset
                           I2C_MEMADD_SIZE_8BIT,
                           (uint8_t *)&edid[i],
                           16, HAL_MAX_DELAY);
         HAL_Delay(2); // kısa gecikme (gerekirse)
     }

     // --- HDMI Map Register Ayarları (I2C addr: 0x68) ---

     // 0x6E = 0x04 → Use internal EDID
     uint8_t reg6E = 0x04;
     HAL_I2C_Mem_Write(&hi2c1, 0x68, 0x6E, I2C_MEMADD_SIZE_8BIT, &reg6E, 1, HAL_MAX_DELAY);

     // 0x6F = 0x0C → Enable EDID + DDC access
     uint8_t reg6F = 0x0C;
     HAL_I2C_Mem_Write(&hi2c1, 0x68, 0x6F, I2C_MEMADD_SIZE_8BIT, &reg6F, 1, HAL_MAX_DELAY);

     // 0xC9 = 0x01 → Force HPD high
     uint8_t regC9 = 0x01;
     HAL_I2C_Mem_Write(&hi2c1, 0x68, 0xC9, I2C_MEMADD_SIZE_8BIT, &regC9, 1, HAL_MAX_DELAY);
 }


 void EDID_Conf (void){
	 HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_KSV_Addr,TX_ADV7611_Buffer55,2,HAL_MAX_DELAY);

	 for (int i = 0; i < 256; i++) {
	     uint8_t data[2] = {i, edid[i]};
	     HAL_I2C_Master_Transmit(&hi2c1, TX_ADV7611_EDID_Addr, data, 2, HAL_MAX_DELAY);
	 }

	 HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_KSV_Addr,TX_ADV7611_Buffer56,2,HAL_MAX_DELAY);

	 uint8_t reg6E = 0x04;
	     HAL_I2C_Mem_Write(&hi2c1, 0x68, 0x6E, I2C_MEMADD_SIZE_8BIT, &reg6E, 1, HAL_MAX_DELAY);

	     // 0x6F = 0x0C → Enable EDID + DDC access
	     uint8_t reg6F = 0x0C;
	     HAL_I2C_Mem_Write(&hi2c1, 0x68, 0x6F, I2C_MEMADD_SIZE_8BIT, &reg6F, 1, HAL_MAX_DELAY);

	     // 0xC9 = 0x01 → Force HPD high
	     uint8_t regC9 = 0x01;
	     HAL_I2C_Mem_Write(&hi2c1, 0x68, 0xC9, I2C_MEMADD_SIZE_8BIT, &regC9, 1, HAL_MAX_DELAY);

 }

 void ADV7611_Unmute(void){

	 HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer41,2,HAL_MAX_DELAY);
 }

 void ADV7611_Reset(void){

	 LL_GPIO_ResetOutputPin(GPIOI, HDMI_RESET_Pin);
 }

uint8_t ADV7611_Init(void){
	uint8_t adv7611_ID = 0;
	//LL_GPIO_ResetOutputPin(GPIOA, HDMI_RESET_Pin);
	//HAL_Delay(6);
	LL_GPIO_SetOutputPin(GPIOI, HDMI_RESET_Pin);
	//HAL_Delay(1);

		HAL_I2C_Master_Transmit(&hi2c1,(TX_ADV7611_Addr),TX_ADV7611_Buffer0,2,HAL_MAX_DELAY);
		HAL_Delay(10);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer1,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer2,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer3,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer4,2,HAL_MAX_DELAY);
	    HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer5,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer6,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer7,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer8,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer9,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer10,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer11,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer12,2,HAL_MAX_DELAY);
	    HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer13,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer14,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer15,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer16,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer17,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_Addr,TX_ADV7611_Buffer18,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,0x44,TX_ADV7611_Buffer19,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,0x64,TX_ADV7611_Buffer20,2,HAL_MAX_DELAY);

	    HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer21,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer22,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer23,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer24,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer25,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer26,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer27,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer28,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer29,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer30,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer31,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer32,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer33,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer34,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer35,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer36,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer37,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer38,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer39,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer40,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer41,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer42,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer43,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer44,2,HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer45,2,HAL_MAX_DELAY);
		//HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer48,2,HAL_MAX_DELAY);

		HAL_I2C_Master_Transmit(&hi2c1, TX_ADV7611_HDMI_Addr, TX_ADV7611_Buffer_Audio1, 2, HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c1, TX_ADV7611_HDMI_Addr, TX_ADV7611_Buffer_Audio2, 2, HAL_MAX_DELAY);

		//adv7611_load_edid();

		//HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer50,2,HAL_MAX_DELAY);
		//HAL_I2C_Master_Transmit(&hi2c1,TX_ADV7611_HDMI_Addr,TX_ADV7611_Buffer52,2,HAL_MAX_DELAY);

	 HAL_I2C_Mem_Read(&hi2c1, TX_ADV7611_Addr, TX_ADV7611_RDINFO_0, I2C_MEMADD_SIZE_8BIT, &RX_ADV7611_RDINFO_lsb, 1, HAL_MAX_DELAY);
	 HAL_I2C_Mem_Read(&hi2c1, TX_ADV7611_Addr, TX_ADV7611_RDINFO_1, I2C_MEMADD_SIZE_8BIT, &RX_ADV7611_RDINFO_msb, 1, HAL_MAX_DELAY);
	 RX_ADV7611_RDINFO = (RX_ADV7611_RDINFO_msb << 8) | RX_ADV7611_RDINFO_lsb;

	  if (RX_ADV7611_RDINFO == 0x2051){
	  	  adv7611_ID=1;
	   }else{
	  	  adv7611_ID=0;
	    }


	  	while(HAL_I2C_IsDeviceReady(&hi2c1, 0x68,5, HAL_MAX_DELAY));

	return adv7611_ID;
}
