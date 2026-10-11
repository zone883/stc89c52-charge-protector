#include "I2C.h"
#include "INA219.h"


#define INA219_ADDRESS 0x40



//写寄存器
void INA219_WriteReg(unsigned char Reg, unsigned int Data)
{
    I2C_Start();

    I2C_SendByte(INA219_ADDRESS << 1);

    I2C_SendByte(Reg);

    I2C_SendByte(Data >> 8);

    I2C_SendByte(Data & 0xFF);

    I2C_Stop();
}



//读寄存器
unsigned int INA219_ReadReg(unsigned char Reg)
{
    unsigned int Data;


    I2C_Start();

    I2C_SendByte(INA219_ADDRESS << 1);

    I2C_SendByte(Reg);



    I2C_Start();

    I2C_SendByte((INA219_ADDRESS << 1) | 1);



    Data = I2C_ReceiveByte();

    Data <<= 8;


    //继续接收第二字节
    I2C_SendAck(0);


    Data |= I2C_ReceiveByte();


    //结束读取
    I2C_SendAck(1);


    I2C_Stop();


    return Data;
}





//检测INA219是否存在
bit INA219_Check(void)
{
    bit Ack;


    I2C_Start();


    Ack = I2C_SendByte(INA219_ADDRESS << 1);


    I2C_Stop();


    return Ack;
}






//初始化INA219

void INA219_Init(void)
{

    /*
    
    配置寄存器0x00

    0x399F:

    - 32V测量范围
    - ±320mV分流范围
    - 连续转换
    
    */

    INA219_WriteReg(0x00,0x399F);



    /*
    
    校准寄存器

    Current_LSB = 0.1mA

    */

    INA219_WriteReg(0x05,4096);

}





//读取总线电压

//返回单位 mV

unsigned int INA219_ReadBusVoltage(void)
{

    unsigned int Data;


    Data = INA219_ReadReg(0x02);


    //右移3位，再乘4

    Data = (Data >> 3) * 4;


    return Data;

}





//读取电流

//返回单位 mA

int INA219_ReadCurrent(void)
{

    int Data;


    Data = (int)INA219_ReadReg(0x04);


    return Data ;

}