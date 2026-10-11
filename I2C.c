#include <REGX52.H>
#include "Delay.h"

sbit I2C_SDA = P1^2;
sbit I2C_SCL = P1^3;

void I2C_Delay(void)
{
    unsigned char i;
    i = 5;
    while(i--);
}

void I2C_Start(void)
{
    I2C_SDA = 1;
    I2C_SCL = 1;
    I2C_Delay();

    I2C_SDA = 0;
    I2C_Delay();

    I2C_SCL = 0;
}

void I2C_Stop(void)
{
    I2C_SDA = 0;
    I2C_SCL = 1;
    I2C_Delay();

    I2C_SDA = 1;
    I2C_Delay();
}

bit I2C_SendByte(unsigned char Byte)
{
    unsigned char i;
    bit Ack;

    for(i = 0; i < 8; i++)
    {
        if(Byte & 0x80)
            I2C_SDA = 1;
        else
            I2C_SDA = 0;

        Byte <<= 1;

        I2C_SCL = 1;
        I2C_Delay();

        I2C_SCL = 0;
        I2C_Delay();
    }

    /* 释放 SDA，让 INA219 回 ACK */
    I2C_SDA = 1;

    I2C_SCL = 1;
    I2C_Delay();

    Ack = I2C_SDA;

    I2C_SCL = 0;
    I2C_Delay();

    return Ack == 0;
}

/* ============================================================
   以下两个函数为补齐实现：
   INA219.c 调用了 I2C_ReceiveByte() 与 I2C_SendAck()，
   但原工程只在 I2C.h 里声明、没有写实现，链接时会出现
   UNRESOLVED EXTERNAL SYMBOL，读寄存器拿到的是无效数据。
   实现照搬本工程里的 I^2C.c，延时沿用本文件的 I2C_Delay()。
   ============================================================ */

unsigned char I2C_ReceiveByte(void)
{
    unsigned char i;
    unsigned char Byte = 0;

    I2C_SDA = 1;

    for(i = 0; i < 8; i++)
    {
        I2C_SCL = 1;
        I2C_Delay();

        Byte <<= 1;
        Byte |= I2C_SDA;

        I2C_SCL = 0;
        I2C_Delay();
    }

    return Byte;
}

void I2C_SendAck(bit AckBit)
{
    I2C_SDA = AckBit;

    I2C_SCL = 1;
    I2C_Delay();

    I2C_SCL = 0;
    I2C_Delay();
}
