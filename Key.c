#include <REGX52.H>
#include "Key.h"

/* ------------------------------------------------------------------
 * 按键引脚映射表  (重要!)
 *
 * 江协科技 / 普中开发板上, 四个独立按键并不是按 P3.0~P3.3 顺序接的,
 * 实际接法是:  K1=P3.1   K2=P3.0   K3=P3.2   K4=P3.3
 * 所以如果按 P3.0~P3.3 顺序去读, 按"K1"会当成 K2, 按"K2"会当成 K1。
 *
 * 下面数组按【丝印 K1,K2,K3,K4】的顺序填对应的 P3 引脚号(0~3)。
 *
 * 换了别的板子怎么确定顺序: 用"按键反馈条"(见 main.c)
 *   按住丝印 K1 不放, 看板载 LED 亮几格:
 *       亮 1 格 -> 映射正确
 *       亮 2 格 -> K1/K2 接反了, 把下面数组前两个数字交换即可
 *   依次试完 4 个键, 就能把这张表填对。
 * ------------------------------------------------------------------ */
static unsigned char code KEY_PIN_MAP[4] = { 1, 0, 2, 3 };

#define KEY_COUNT       4
#define KEY_DEBOUNCE    2       /* 2  * 10ms = 20ms 消抖 */
#define KEY_LONG_TIME   100     /* 100 * 10ms = 1s  判长按 */

static unsigned char key_down[KEY_COUNT];      /* 已确认按下 */
static unsigned int  key_time[KEY_COUNT];      /* 按住时长(10ms 为单位) */
static unsigned char key_long_sent[KEY_COUNT]; /* 长按事件是否已发过 */
static unsigned char key_event = 0;

/* 返回 1 表示该键被按下 (引脚低电平 = 按下) */
static unsigned char Key_ReadPin(unsigned char n)
{
    unsigned char pin;

    pin = KEY_PIN_MAP[n - 1];

    if(P3 & (unsigned char)(1u << pin)) return 0;   /* 高电平: 松开 */
    return 1;                                       /* 低电平: 按下 */
}

void Key_Update(void)
{
    unsigned char i, n, now;

    for(i = 0; i < KEY_COUNT; i++)
    {
        n   = (unsigned char)(i + 1);
        now = Key_ReadPin(n);

        if(now)
        {
            if(key_time[i] < 60000) key_time[i]++;

            /* 消抖通过 -> 确认按下 */
            if((key_time[i] >= KEY_DEBOUNCE) && (!key_down[i]))
            {
                key_down[i]      = 1;
                key_long_sent[i] = 0;
            }

            /* 按住满 1 秒 -> 上报长按 */
            if(key_down[i] && (!key_long_sent[i]) && (key_time[i] >= KEY_LONG_TIME))
            {
                key_long_sent[i] = 1;
                key_event = (unsigned char)(n | KEY_LONG_FLAG);
            }
        }
        else
        {
            /* 松手: 如果之前没发过长按, 就上报一次短按 */
            if(key_down[i] && (!key_long_sent[i]))
            {
                key_event = n;
            }

            key_down[i]      = 0;
            key_time[i]      = 0;
            key_long_sent[i] = 0;
        }
    }
}

unsigned char Key_GetEvent(void)
{
    unsigned char e;

    e = key_event;
    key_event = 0;

    return e;
}

/* 当前按住的按键掩码: bit0~bit3 对应 K1~K4
 * 用来做"按键反馈条": 按住哪个键, 板载 LED 就亮几格 */
unsigned char Key_GetDown(void)
{
    unsigned char i, m;

    m = 0;
    for(i = 0; i < KEY_COUNT; i++)
    {
        if(key_down[i]) m |= (unsigned char)(1u << i);
    }
    return m;
}
