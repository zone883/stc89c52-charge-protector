#ifndef __INA219_H__
#define __INA219_H__


void INA219_Init(void);

int INA219_ReadCurrent(void);

unsigned int INA219_ReadBusVoltage(void);

bit INA219_Check(void);


#endif