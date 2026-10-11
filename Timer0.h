#ifndef __TIMER0_H__
#define __TIMER0_H__

/* 系统 1ms 节拍, 用于: 充电计时 / 按键扫描 / 传感器采样调度 */
void Timer0_Init(void);
unsigned int Timer0_GetTick(void);      /* 返回上电以来经过的毫秒数(约 65 秒回绕) */

#endif
