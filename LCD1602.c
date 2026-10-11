#include <REGX52.H>
#include "LCD1602.h"

/* 引脚配置 */
sbit LCD_RS = P2^6;
sbit LCD_RW = P2^5;
sbit LCD_EN = P2^7;
#define LCD_DataPort P0

/* ------------------------------------------------------------------
 * 约 150us 延时 (11.0592MHz / 12MHz 晶振)。
 * HD44780 一次写入只需要 40us, 这里留了 3 倍以上余量, 写屏速度也够快。
 * (清屏命令要 1.64ms, 所以 LCD_Init 和 LCD_Check 里另外补了延时)
 * ------------------------------------------------------------------ */
static void LCD_Delay(void)
{
    unsigned char i;

    i = 250;
    while(i--);
}

/* 写命令 */
void LCD_WriteCommand(unsigned char Command)
{
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_DataPort = Command;
    LCD_EN = 1;
    LCD_Delay();
    LCD_EN = 0;
    LCD_Delay();
}

/* 写数据 */
void LCD_WriteData(unsigned char Data)
{
    LCD_RS = 1;
    LCD_RW = 0;
    LCD_DataPort = Data;
    LCD_EN = 1;
    LCD_Delay();
    LCD_EN = 0;
    LCD_Delay();
}

/* 读回地址计数器(顺便把忙标志读回来) */
static unsigned char LCD_ReadStatus(void)
{
    unsigned char st;

    LCD_DataPort = 0xFF;        /* P0 释放, 靠上拉电阻保持高电平 */
    LCD_RS = 0;
    LCD_RW = 1;
    LCD_EN = 1;
    LCD_Delay();
    st = LCD_DataPort;
    LCD_EN = 0;
    LCD_RW = 0;
    LCD_Delay();

    return st;
}

/* ------------------------------------------------------------------
 * 探测 LCD 到底接没接上:
 * 发一条清屏命令后读回地址计数器, 正常应读到 0x80 (忙标志 0 + AC = 0);
 * 屏幕没插好时 P0 被上拉电阻拉成 0xFF, 由此判断"屏不在"。
 * 这样即使 LCD 接线有问题, 程序也能自动改用点阵显示, 不会一片空白。
 * ------------------------------------------------------------------ */
bit LCD_Check(void)
{
    unsigned int i;

    LCD_WriteCommand(0x01);         /* 清屏 */
    for(i = 0; i < 40; i++)         /* 清屏要 1.64ms, 这里等到位 */
    {
        LCD_Delay();
    }

    if(LCD_ReadStatus() == 0x80) return 1;
    return 0;
}

/* 设置光标位置: Line 1~2, Column 1~16 */
void LCD_SetCursor(unsigned char Line, unsigned char Column)
{
    if(Line == 1)
    {
        LCD_WriteCommand(0x80 | (Column - 1));
    }
    else
    {
        LCD_WriteCommand(0x80 | (Column - 1 + 0x40));
    }
}

/* 初始化: 八位数据接口, 两行显示, 5x7 点阵 */
void LCD_Init(void)
{
    unsigned int i;

    LCD_WriteCommand(0x38);
    LCD_WriteCommand(0x0C);         /* 显示开, 光标关 */
    LCD_WriteCommand(0x06);         /* 写入后地址自动加一 */
    LCD_WriteCommand(0x01);         /* 清屏 */
    for(i = 0; i < 40; i++)
    {
        LCD_Delay();
    }
}

void LCD_ShowString(unsigned char Line, unsigned char Column, char *String)
{
    unsigned char i;

    LCD_SetCursor(Line, Column);
    for(i = 0; String[i] != '\0'; i++)
    {
        LCD_WriteData(String[i]);
    }
}
