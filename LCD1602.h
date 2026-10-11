#ifndef __LCD1602_H__
#define __LCD1602_H__

#include <REGX52.H>

/* LCD1602: RS=P2.6  RW=P2.5  EN=P2.7  D0~D7=P0 */
void LCD_Init(void);
bit  LCD_Check(void);      /* 开机探测屏幕接没接上: 1 = 有应答 */

void LCD_ShowString(unsigned char Line, unsigned char Column, char *String);
void LCD_SetCursor(unsigned char Line, unsigned char Column);

#endif
