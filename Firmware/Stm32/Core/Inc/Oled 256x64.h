
#define uint unsigned int
#define uchar unsigned char
#include "u8g.h"	
void test(void);	
void Clear_ram(void);
void Oled_Init(void);
void Write_number(uchar k,uchar station_dot);
void display_Contrast_level(uchar number);
void adj_Contrast(void);
void Delay(uint n);
void Write_Data(unsigned char dat);
void Write_Datau(unsigned char dat);
void Write_Command(unsigned char cmd);
void Set_Row_Address(unsigned char add);
void Set_Column_Address(unsigned char add);
void Set_Contrast_Control_Register(unsigned char mod);
void Display_Chess(unsigned char value1,unsigned char value2);
void Display_Picture(unsigned char pic[]);
void DrawString(uint x, uint y, char *pStr);
void DrawSingleAscii(uint x, uint y, char *pAscii);
void Gray_test(void);
void Data_processing(uchar temp);
void Draw_Rectangle(unsigned char Data, unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e);
void ShowVolBar(unsigned char bar);
void numToLcdG(unsigned char num, unsigned char x, unsigned char y);
void Clear_Glcd(void);
void ShowVol(char vol);
void ShowVolCd(char vol);
void numToLcdGt(long int num);
void Num1 (uint x, uint y, uchar number);
void Big_Font (uint x, uint y, char *pAscii);
void DrawBigFontString(uint x, uint y, char *pStr);
void Arc (uint x, uint y, uchar number);
void numToLcdGk(unsigned char number, unsigned char x, unsigned char y);
void numToLcdGkD(unsigned char number, unsigned char x, unsigned char y);
void clrARC (void);
void numToLcdSampling(unsigned char number, unsigned char x, unsigned char y);
void Write_number_Small(uchar k,uchar col, uchar row);
void display_CS8416_Sampling(unsigned char sampling,unsigned char wordlenght, unsigned char x, unsigned char y);
void display_small_BCD(unsigned char number, unsigned char x, unsigned char y );
void Numu8g (u8g_t u8g, uint x, uint y, uchar number);
void numToLcdGu8g(u8g_t u8g, unsigned char number, unsigned char x, unsigned char y);
void Show_Time(u8g_t u8g);
//
