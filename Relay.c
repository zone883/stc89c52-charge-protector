#include <REGX52.H>
#include "Relay.h"
#include "Delay.h"

/* ------------------------------------------------------------------
 * 继电器控制
 *
 * 触发极性: 绝大多数"5V 单路继电器模块(带光耦隔离)"都是低电平触发,
 *           也就是 IN 接低电平时继电器吸合。
 *           0 = 高电平触发      1 = 低电平触发
 * 如果你的模块是高电平触发, 把下面这行改成 0
 *
 * 判断方法: 开机自检时会"咔哒"两下, 如果方向反了(通电状态和你看到的相反),
 *           就把这个宏取反即可
 * ------------------------------------------------------------------ */
#define RELAY_ACTIVE_LOW 1

sbit Relay_Pin = P1^0;                   /* 换引脚只改这一行 */

static unsigned char relay_on = 0;

void Relay_Set(unsigned char on)
{
    if(on) relay_on = 1;
    else   relay_on = 0;

#if RELAY_ACTIVE_LOW
    if(relay_on) Relay_Pin = 0;
    else         Relay_Pin = 1;
#else
    if(relay_on) Relay_Pin = 1;
    else         Relay_Pin = 0;
#endif
}

unsigned char Relay_GetState(void)
{
    return relay_on;
}

void Relay_Init(void)
{
    Relay_Set(0);                        /* 上电默认断开, 更安全 */
}

/* 开机自检: 吸合-断开-吸合-断开, 听到两下"咔哒"说明继电器通路正常 */
void Relay_SelfTest(void)
{
    Relay_Set(1); Delay(250);
    Relay_Set(0); Delay(250);
    Relay_Set(1); Delay(250);
    Relay_Set(0); Delay(250);
}
