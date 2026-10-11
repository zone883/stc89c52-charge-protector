#include "I2C.h"
#include "INA219.h"

#define INA219_ADDRESS      0x40
#define INA219_CURRENT_DIV  10      /* Current_LSB = 0.1mA, 换成 mA 要除以 10 */

/* 器件是否在线的标志 */
static bit ina_ok = 0;

/* 写 16 位寄存器 */
static void INA219_WriteReg(unsigned char Reg, unsigned int Data)
{
    I2C_Start();
    I2C_SendByte((unsigned char)(INA219_ADDRESS << 1));
    I2C_SendByte(Reg);
    I2C_SendByte((unsigned char)(Data >> 8));
    I2C_SendByte((unsigned char)(Data & 0xFF));
    I2C_Stop();
}

/* 读 16 位寄存器 */
static unsigned int INA219_ReadReg(unsigned char Reg)
{
    unsigned int Data;

    I2C_Start();
    I2C_SendByte((unsigned char)(INA219_ADDRESS << 1));
    I2C_SendByte(Reg);

    I2C_Start();                                    /* 重复起始条件 */
    I2C_SendByte((unsigned char)((INA219_ADDRESS << 1) | 0x01));

    Data  = I2C_ReceiveByte();
    Data <<= 8;
    I2C_SendAck(0);                                 /* ACK: 继续读第二字节 */
    Data |= I2C_ReceiveByte();
    I2C_SendAck(1);                                 /* NACK: 读完了 */

    I2C_Stop();

    return Data;
}

/* 探测器件: 发地址看有没有 ACK */
bit INA219_Check(void)
{
    bit ack;

    I2C_Start();
    ack = I2C_SendByte((unsigned char)(INA219_ADDRESS << 1));
    I2C_Stop();

    if(ack) return 1;
    return 0;
}

void INA219_Init(void)
{
    unsigned int cfg;

    I2C_BusRecover();                   /* 先救一下可能被拉死的总线 */

    ina_ok = INA219_Check();
    if(!ina_ok)
        return;

    /* 配置寄存器 0x00 = 0x399F
     *   bit13 BRNG = 0    -> 总线电压量程 16V
     *   bit12~11 PG = 11  -> 分流电压 ±320mV
     *   ADC 12 位, 采样 128 次取平均
     *   MODE = 111        -> 电压电流连续转换 */
    INA219_WriteReg(0x00, 0x399F);

    /* 校准寄存器 0x05 = 4096, 对应 0.1mA/LSB (分流电阻 0.1Ω) */
    INA219_WriteReg(0x05, 4096);

    /* 回读配置寄存器验证一次, 写不进去说明 I2C 还是不通 */
    cfg = INA219_ReadReg(0x00);
    if(cfg != 0x399F)
    {
        ina_ok = 0;
    }
}

/* 总线电压, 单位 mV: 寄存器 bit15~bit3, 最小刻度 4mV */
unsigned int INA219_ReadBusVoltage(void)
{
    unsigned int Data;

    Data = INA219_ReadReg(0x02);
    Data = (Data >> 3) * 4;

    return Data;
}

/* 电流, 单位 mA: 电流寄存器原始值 * 0.1mA = 原始值/10 */
int INA219_ReadCurrent(void)
{
    int Data;

    Data = (int)INA219_ReadReg(0x04);
    Data = Data / INA219_CURRENT_DIV;

    return Data;
}

/* 一次读完电压+电流, 并顺手判断器件是不是读着读着掉线了 */
bit INA219_ReadAll(unsigned int *mV, int *mA)
{
    unsigned int v;

    *mV = 0;
    *mA = 0;

    if(!ina_ok)
        return 0;

    v = INA219_ReadBusVoltage();

    /* 全是 1 说明总线上根本没人应答(SDA 被上拉电阻拉高), 视为掉线 */
    if(v >= 32000)
    {
        if(!INA219_Check())
        {
            ina_ok = 0;
            return 0;
        }
    }

    *mV = v;
    *mA = INA219_ReadCurrent();

    return 1;
}
