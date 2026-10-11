#include <REGX52.H>
#include "I2C.h"

/* ------------------------------------------------------------------
 * 软件模拟 I2C 主机
 * SDA = P1.2   SCL = P1.3      <-- 换引脚只改这两行
 * ------------------------------------------------------------------ */
sbit I2C_SDA = P1^2;
sbit I2C_SCL = P1^3;

/* 约 5us 延时, 总线速率 100kHz 左右, INA219 完全能吃下 */
static void I2C_Delay(void)
{
    unsigned char i;
    i = 5;
    while(i--);
}

/* 总线恢复: 从机如果因为掉电/干扰把 SDA 拉死, 用 9 个时钟把它顶出来 */
void I2C_BusRecover(void)
{
    unsigned char i;

    I2C_SDA = 1;
    I2C_SCL = 1;
    I2C_Delay();

    for(i = 0; i < 9; i++)
    {
        I2C_SCL = 0;
        I2C_Delay();
        I2C_SCL = 1;
        I2C_Delay();
    }

    /* 补一个停止条件, 让总线回到空闲 */
    I2C_SDA = 0;
    I2C_Delay();
    I2C_SCL = 1;
    I2C_Delay();
    I2C_SDA = 1;
    I2C_Delay();
}

void I2C_Start(void)
{
    I2C_SDA = 1;
    I2C_SCL = 1;
    I2C_Delay();

    I2C_SDA = 0;
    I2C_Delay();

    I2C_SCL = 0;
    I2C_Delay();
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

    /* 释放 SDA, 让从机回 ACK */
    I2C_SDA = 1;

    I2C_SCL = 1;
    I2C_Delay();
    Ack = I2C_SDA;
    I2C_SCL = 0;
    I2C_Delay();

    if(Ack == 0) return 1;      /* 从机把 SDA 拉低 = 应答 */
    return 0;
}

unsigned char I2C_ReceiveByte(void)
{
    unsigned char i;
    unsigned char Byte = 0;

    I2C_SDA = 1;                /* 主机释放总线, 交给从机驱动 */

    for(i = 0; i < 8; i++)
    {
        I2C_SCL = 1;
        I2C_Delay();

        Byte <<= 1;
        if(I2C_SDA) Byte |= 0x01;

        I2C_SCL = 0;
        I2C_Delay();
    }

    return Byte;
}

void I2C_SendAck(bit AckBit)
{
    if(AckBit)
        I2C_SDA = 1;            /* NACK: 最后一个字节读完 */
    else
        I2C_SDA = 0;            /* ACK : 还要继续读 */

    I2C_SCL = 1;
    I2C_Delay();
    I2C_SCL = 0;
    I2C_Delay();
}
