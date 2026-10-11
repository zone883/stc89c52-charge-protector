#ifndef __I2C_H__
#define __I2C_H__

#include <REGX52.H>

/* 软件模拟 I2C 主机
 * SDA = P1.2   SCL = P1.3
 * 如果 INA219 实际接在别的引脚上, 只改 I2C.c 顶部那两行 sbit
 */
void I2C_BusRecover(void);              /* 总线被拉死时恢复: 打 9 个时钟 + 停止位 */
void I2C_Start(void);
void I2C_Stop(void);
bit  I2C_SendByte(unsigned char Byte);  /* 返回值: 1 = 从机应答(ACK) */
unsigned char I2C_ReceiveByte(void);
void I2C_SendAck(bit AckBit);           /* 0 = ACK(继续读)  1 = NACK(结束) */

#endif
