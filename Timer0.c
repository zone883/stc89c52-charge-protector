#include <REGX52.H>
#include "Timer0.h"

/* ------------------------------------------------------------------
 * 定时器0 每 1ms 产生一次中断, 作为整个程序的"心跳"
 *
 * 晶振 12MHz   : 重装值 65536 - 1000 = 0xFC18
 * 晶振 11.0592 : 重装值 65536 -  921 = 0xFC67
 * 两者只差 8%, 对秒表显示没有影响, 默认按 12MHz 算;
 * 如果你的板子是 11.0592MHz 晶振, 把下面这行改成 1
 *
 * 计数器用 16 位(0~65535ms, 约 65 秒回绕一次)。
 * 主循环里一律用 "now - last >= 间隔" 的写法, 回绕也是安全的。
 * ------------------------------------------------------------------ */
#define FOSC_110592 0

#if FOSC_110592
  #define T0_H 0xFC
  #define T0_L 0x67
#else
  #define T0_H 0xFC
  #define T0_L 0x18
#endif

static volatile unsigned int g_ms = 0;

void Timer0_Init(void)
{
    TMOD &= 0xF0;           /* 定时器0 工作在方式1(16位) */
    TMOD |= 0x01;

    TH0 = T0_H;
    TL0 = T0_L;

    TF0 = 0;
    TR0 = 1;                /* 启动定时器 */
    ET0 = 1;                /* 允许定时器0中断 */
    EA  = 1;                /* 开总中断 */
}

unsigned int Timer0_GetTick(void)
{
    unsigned int t;

    /* 读 16 位变量时关一下中断, 防止读到"半新半旧"的值 */
    ET0 = 0;
    t = g_ms;
    ET0 = 1;

    return t;
}

void Timer0_ISR(void) interrupt 1
{
    TH0 = T0_H;
    TL0 = T0_L;
    g_ms++;
}
