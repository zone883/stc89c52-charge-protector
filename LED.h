#ifndef __LED_H__
#define __LED_H__

#include <REGX52.H>

/* ------------------------------------------------------------------
 * 板载 8 个 LED 做"电流柱状条"
 *   (江协科技/普中 A2 板: 8 个 LED 全接在 P2.0~P2.7, 共阳接法, 低电平点亮)
 *
 * 显示规则:
 *   IDLE    (空闲)     全灭
 *   CHARGING(充电中)   按电流大小点亮 1~8 格(以过流阈值为满格), 直观显示 INA219 读数
 *   FULL    (已充满)   8 格全亮
 *   OVERHEAT/OCURRENT  全灭 <-> 全亮 交替闪烁(告警, 250ms 一次)
 *   开机               流水灯 1 格 -> 8 格, 再全亮一下, 顺便验灯
 *
 * 注意: 板载 LED 和 LCD 控制线共用 P2.5/P2.6/P2.7
 *       => 用 LED 柱状条时就不能再用 LCD1602 当显示器(本作品 LCD 是当负载用的, 不冲突)
 *          如果以后想 LCD 和 LED 同时用, 把下面 LED_BAR_N 改成 5(只用 P2.0~P2.4)
 * 另外 P2.5 在很多板子上是蜂鸣器: 亮到第 6 格可能会响, 不想要就拔掉蜂鸣器跳线帽
 * ------------------------------------------------------------------ */
#define LED_BAR_N   8      /* 8 = 用全部 8 格   5 = 只用 P2.0~P2.4(避开蜂鸣器与 LCD) */

void LED_Init(void);                                            /* 开机流水灯自检 */
void LED_Bar(unsigned char n);                                  /* 点亮 n 格 (0~LED_BAR_N) */
void LED_Show(unsigned char state, unsigned char blink, unsigned char level);
/* state: 0=空闲 1=充电中 2=已充满 3=过温 4=过流跳闸 */
/* blink: 每 250ms 翻转一次, 用来让告警闪起来   level: 0~8 柱状条格数 */

#endif
