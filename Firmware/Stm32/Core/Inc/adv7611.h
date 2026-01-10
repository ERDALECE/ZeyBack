/*
 * adv7611.h
 *
 *  Created on: Mar 21, 2024
 *      Author: erdal
 */

#ifndef INC_ADV7611_H_
#define INC_ADV7611_H_

uint8_t ADV7611_Init(void);
void ADV7611_Unmute(void);
void ADV7611_Reset(void);
void EDID_Conf (void);
void adv7611_load_edid(void);


#endif /* INC_ADV7611_H_ */
