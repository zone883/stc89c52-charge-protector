#include <REGX52.H>
#include "LED.h"
#include "Delay.h"

/* 板载 LED 全部挂在 P2 口, 共阳接法: 引脚输出低电平 -> 灯亮 */
#define LED_PORT P2

#if LED_BAR_N >= 8
  #define LED_MASK 0xFF            /* 8 格: P2.0~P2.7 */
#else
  #define LED_MASK 0x1F            /* 5 格: P2.0~P2.4, 避开蜂鸣器/ LCD 控制线 */
#endif

/* ------------------------------------------------------------------
 * 点亮从 P2.0 开始连续 n 格 (n = 0~LED_BAR_N)
 * 只改动 LED 用到的那些位, 不影响同一个口上的其它功能
 * ------------------------------------------------------------------ */
void LED_Bar(unsigned char n)
{
    unsigned char i;
    unsigned char mask = 0;

    for(i = 0; i < 8; i++)
    {
        if(LED_MASK & (unsigned char)(1u << i))
        {
            if(n == 0) break;
            mask |= (unsigned char)(1u << i);
            n--;
        }
    }

    /* 低电平点亮: 应该在亮的位置输出 0, 其余保持原样 */
    LED_PORT = (unsigned char)((LED_PORT & (unsigned char)~LED_MASK)
                             | ((unsigned char)~mask & LED_MASK));
}

/* 开机自检: 1 格 -> 8 格流水, 再全亮一下, 一眼看出有没有坏灯 */
void LED_Init(void)
{
    unsigned char i;

    LED_Bar(0);

    for(i = 1; i <= LED_BAR_N; i++)
    {
        LED_Bar(i);
        Delay(100);
    }

    LED_Bar(0);
    Delay(150);

    LED_Bar(LED_BAR_N);
    Delay(300);

    LED_Bar(0);
}

/* ------------------------------------------------------------------
 * 按当前状态刷新 LED
 *   state: 0=空闲 1=充电中 2=已充满 3=过温 4=过流跳闸
 *   level: 充电中时的柱状格数(0~8), 由主程序按 INA219 电流算出
 * ------------------------------------------------------------------ */
void LED_Show(unsigned char state, unsigned char blink, unsigned char level)
{
    switch(state)
    {
        case 1:                             /* 充电中: 电流条, 至少 1 格 */
            if(level == 0) level = 1;
            LED_Bar(level);
            break;

        case 2:                             /* 已充满: 全亮 */
            LED_Bar(LED_BAR_N);
            break;

        case 3:                             /* 过温保护   */
        case 4:                             /* 过流跳闸   : 全灭/全亮交替闪 */
            if(blink) LED_Bar(LED_BAR_N);
            else      LED_Bar(0);
            break;

        default:                            /* 空闲: 全灭 */
            LED_Bar(0);
            break;
    }
}
