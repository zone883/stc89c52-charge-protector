#ifndef __INA219_H__
#define __INA219_H__

#include <REGX52.H>

/* ------------------------------------------------------------------
 * INA219 双向电流/电压检测模块  (I2C 地址 0x40, 见 I2C.c)
 *
 * 分流电阻: 模块上一般是 0.1Ω (丝印 R100)
 * 校准值 4096 -> Current_LSB = 0.1mA, 所以电流值最后要除以 10 才是 mA
 * ------------------------------------------------------------------ */
bit INA219_Check(void);                   /* 1 = 器件应答, 0 = 找不到器件 */
void INA219_Init(void);
bit INA219_ReadAll(unsigned int *mV, int *mA);   /* 一次读完电压和电流, 1 = 数据可信 */

unsigned int INA219_ReadBusVoltage(void);        /* 总线电压, 单位 mV */
int INA219_ReadCurrent(void);                    /* 电流, 单位 mA */

#endif
