#include <REGX52.H>
#include "Delay.h"
sbit RCK = P3^5;		//RCLK
sbit SCK = P3^6;		//SRCLK
sbit SER = P3^4;		//SER


void _74HC595_WriteByte(unsigned char Byte)
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		SER=(Byte&0x80>>i);			//取出Byte的每一位，&有0则0，若Byte第八位为0，则SER为0，为1则SER为1(多位赋给1位非0则1)
		SCK=1;
		SCK=0;
	}
	RCK=1;
	RCK=0;
}

void MatrixLED_Init()
{
	SER=0;							//P3口上电为1，所以要先在main函数置0
	RCK=0;
}


void MatrixLED_ShowColumn(unsigned char Column,unsigned char Data){
		_74HC595_WriteByte(Data);
		P0=~(0x80>>Column);
		Delay(1);							//消影
		P0=0xFF;
}
