#ifndef __RELAY_H__
#define __RELAY_H__

void Relay_Init(void);
void Relay_Set(unsigned char on);        /* 1 = 导通(允许充电)  0 = 断开 */
unsigned char Relay_GetState(void);      /* 返回当前通断状态 */
void Relay_SelfTest(void);               /* 开机自检: "咔哒"两下, 方便确认接线 */

#endif
