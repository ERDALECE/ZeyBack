/*
 * adv7611_config.h
 *
 *  Created on: Mar 21, 2024
 *      Author: erdal
 */

#ifndef INC_ADV7611_CONFIG_H_
#define INC_ADV7611_CONFIG_H_

#define TX_ADV7611_Addr        0x98
#define TX_ADV7611_HDMI_Addr   0x68
#define TX_ADV7611_DPLL_Addr   0x4C
#define TX_ADV7611_EDID_Addr   0x6C
#define TX_ADV7611_CP_Addr     0x44
#define TX_ADV7611_KSV_Addr    0x64
#define TX_ADV7611_CEC_Addr    0x80
#define TX_ADV7611_INFO_Addr   0x7C

uint8_t TX_ADV7611_RDINFO_0={0xEB};
uint8_t TX_ADV7611_RDINFO_1={0xEA};
uint8_t RX_ADV7611_RDINFO_lsb;
uint8_t RX_ADV7611_RDINFO_msb;
uint16_t RX_ADV7611_RDINFO;

// I2S Word Length = 24 bit, 4 channel mode
uint8_t TX_ADV7611_Buffer_Audio1[2] = {0x15, 0xB0};

// I2S Master mode disable, WS polarity, left-justified
uint8_t TX_ADV7611_Buffer_Audio2[2] = {0x16, 0x60};



uint8_t TX_ADV7611_Buffer0 [2] = {0xFF, 0x80};//I2C reset
uint8_t TX_ADV7611_Buffer02 [2] = {0x17, 0x02};//I2C reset
uint8_t TX_ADV7611_Buffer01 [2] = {0xFF, 0x00};//I2C reset
uint8_t TX_ADV7611_Buffer00 [2] = {0x00, 0x1E};//I2C reset
uint8_t TX_ADV7611_Buffer1 [2] = {0xF4, 0x80};//CEC
uint8_t TX_ADV7611_Buffer2 [2] = {0xF5, 0x7C};//INFOFRAME
uint8_t TX_ADV7611_Buffer3 [2] = {0xF8, 0x4C};//DPLL
uint8_t TX_ADV7611_Buffer4 [2] = {0xF9, 0x64};//KSV
uint8_t TX_ADV7611_Buffer5 [2] = {0xFA, 0x6C};//EDID
uint8_t TX_ADV7611_Buffer6 [2] = {0xFB, 0x68};//HDMI
uint8_t TX_ADV7611_Buffer7 [2] = {0xFD, 0x44};//CP
uint8_t TX_ADV7611_Buffer8 [2] = {0x01, 0x06};//Prim_Mode =110b HDMI-GR
uint8_t TX_ADV7611_Buffer9 [2] = {0x02, 0xF5};//Auto CSC, YCrCb out, Set op_656 bit
uint8_t TX_ADV7611_Buffer10 [2] = {0x03, 0x40};//24 bit SDR 444 Mode 0
uint8_t TX_ADV7611_Buffer11 [2] = {0x05, 0x2C};//AV Codes Off
uint8_t TX_ADV7611_Buffer12 [2] = {0x06, 0xA6};//Invert VS,HS pins
uint8_t TX_ADV7611_Buffer13 [2] = {0x0B, 0x46};//Power up part
uint8_t TX_ADV7611_Buffer14 [2] = {0x0C, 0x47};//Power up part
uint8_t TX_ADV7611_Buffer15 [2] = {0x14, 0x7F};//Max Drive Strength
uint8_t TX_ADV7611_Buffer16 [2] = {0x15, 0xAE};//Disable Tristate of Pins
uint8_t TX_ADV7611_Buffer17 [2] = {0x19, 0x83};//LLC DLL phase
uint8_t TX_ADV7611_Buffer18 [2] = {0x33, 0x40};//LLC DLL enable

/* Set HDMI FreeRun - 0x44 addr*/
uint8_t TX_ADV7611_Buffer19 [2] = {0xBA, 0x01};//Set HDMI FreeRun

/* Disable HDCP 1.1 features - 0x64 addr*/
uint8_t TX_ADV7611_Buffer20 [2] = {0x40, 0x81};//Disable HDCP 1.1 features


/* ADI recommended setting - 0x68 addr*/
uint8_t TX_ADV7611_Buffer21 [2] = {0x9B, 0x03};
uint8_t TX_ADV7611_Buffer22 [2] = {0xC1, 0x01};
uint8_t TX_ADV7611_Buffer23 [2] = {0xC2, 0x01};
uint8_t TX_ADV7611_Buffer24 [2] = {0xC3, 0x01};
uint8_t TX_ADV7611_Buffer25 [2] = {0xC4, 0x01};
uint8_t TX_ADV7611_Buffer26 [2] = {0xC5, 0x01};
uint8_t TX_ADV7611_Buffer27 [2] = {0xC6, 0x01};
uint8_t TX_ADV7611_Buffer28 [2] = {0xC7, 0x01};
uint8_t TX_ADV7611_Buffer29 [2] = {0xC8, 0x01};
uint8_t TX_ADV7611_Buffer30 [2] = {0xC9, 0x01};
uint8_t TX_ADV7611_Buffer31 [2] = {0xCA, 0x01};
uint8_t TX_ADV7611_Buffer32 [2] = {0xCB, 0x01};
uint8_t TX_ADV7611_Buffer33 [2] = {0xCC, 0x01};

uint8_t TX_ADV7611_Buffer34 [2] = {0x00, 0x00};//Set HDMI Input Port A
uint8_t TX_ADV7611_Buffer35 [2] = {0x83, 0xFE};//Enable clock terminator for port A

uint8_t TX_ADV7611_Buffer36 [2] = {0x6F, 0x0C};//ADI recommended setting
uint8_t TX_ADV7611_Buffer37 [2] = {0x85, 0x1F};//ADI recommended setting
uint8_t TX_ADV7611_Buffer38 [2] = {0x87, 0x70};//ADI recommended setting

uint8_t TX_ADV7611_Buffer39 [2] = {0x8D, 0x04};//LFG
uint8_t TX_ADV7611_Buffer40 [2] = {0x8E, 0x1E};//HFG
uint8_t TX_ADV7611_Buffer41 [2] = {0x1A, 0x8A};//unmute audio

uint8_t TX_ADV7611_Buffer42 [2] = {0x57, 0xDA};//ADI recommended setting
uint8_t TX_ADV7611_Buffer43 [2] = {0x58, 0x01};//ADI recommended setting

uint8_t TX_ADV7611_Buffer44 [2] = {0x03, 0x98};//DIS_I2C_ZERO_COMPR
uint8_t TX_ADV7611_Buffer45 [2] = {0x75, 0x10};//DDC drive strength


uint8_t TX_ADV7611_Buffer46 [2] = {0x6E, 0x00};
uint8_t TX_ADV7611_Buffer47 [2] = {0x4C, 0x44};
uint8_t TX_ADV7611_Buffer48 [2] = {0x6C, 0x01};
uint8_t TX_ADV7611_Buffer49 [2] = {0x20, 0x80};
uint8_t TX_ADV7611_Buffer50 [2] = {0x5A, 0x01};
uint8_t TX_ADV7611_Buffer51 [2] = {0x56, 0xD8};
uint8_t TX_ADV7611_Buffer52 [2] = {0x48, 0x40};
uint8_t TX_ADV7611_Buffer53 [2] = {0x40, 0x80};
uint8_t TX_ADV7611_Buffer54 [2] = {0x01, 0x02};


uint8_t TX_ADV7611_Buffer55 [2] = {0x74, 0x00};
uint8_t TX_ADV7611_Buffer56 [2] = {0x74, 0x03};
uint8_t TX_ADV7611_Buffer57 [2] = {0x53, 0x00};
uint8_t TX_ADV7611_Buffer58 [2] = {0x70, 0x9E};
uint8_t TX_ADV7611_Buffer59 [2] = {0x74, 0x03};


//uint8_t TX_ADV7611_Buffer62 [1] = {0xEA};
//uint8_t TX_ADV7611_Buffer63 [1] = {0xEB};


#endif /* INC_ADV7611_CONFIG_H_ */
