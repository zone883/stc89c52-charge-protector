#include <REGX52.H>

#include "Delay.h"
#include "Timer0.h"
#include "Key.h"
#include "Relay.h"
#include "DS18B20.h"
#include "INA219.h"
#include "LCD1602.h"
#include "MatrixLED.h"
#include "LED.h"

/* ==========================================================================
 *                          充 电 保 护 器
 * ==========================================================================
 * 基于 51 单片机(STC89C52)的充电保护装置:
 * 用 INA219 实时测量充电回路的电压 / 电流, 电流异常时用继电器切断充电回路。
 *
 * 功能:
 *   1. 实时测量: INA219 测负载电压 / 电流, LCD 显示已充电时间
 *   2. 阈值设置: 独立按键调整"充满判定电流"和"过流保护电流"两个阈值
 *   3. 过流保护: 电流超过上限持续 1 秒 -> 继电器跳闸断电, 自锁, 需按 K4 复位
 *               (像家里的空气开关)
 *   4. 充满判定: 电流低于阈值持续 5 秒 -> 自动断电
 *              (不用"电流 == 0"判满, 因为充电末期电流只衰减到几十毫安, 到不了 0)
 *   5. 温度保护: DS18B20 过温断电 + 回差恢复。代码完整保留,
 *               因传感器硬件故障(引脚松)暂用 TEMP_SENSOR_EN = 0 关闭
 *   6. 传感器掉线自动检测 + 每 2 秒重试初始化
 *
 * 引脚分配(要改引脚只需改对应模块文件顶部那一行):
 *   继电器          P1.0       (低电平触发, 见 Relay.c)
 *   INA219 SDA/SCL  P1.2/P1.3  (见 I2C.c)
 *   DS18B20 DQ      P3.7       (见 OneWire.c)
 *   独立按键 K1~K4  P3.1/P3.0/P3.2/P3.3 (开发板丝印顺序, 映射表见 Key.c 顶部)
 *   板载 8 个 LED   P2.0~P2.7(低电平点亮): 充电中按电流亮柱状条,
 *                   已充满全亮, 保护动作时整排闪烁 (见 LED.c)
 *   LCD1602         RS=P2.6 RW=P2.5 EN=P2.7 D0~D7=P0
 *   点阵 74HC595    SER=P3.4 RCK=P3.5 SCK=P3.6 列=P0
 *
 *   LCD 和点阵共用 P0 数据线, 实际只能接一块; 程序开机自动探测,
 *   读到 LCD 应答就用 LCD, 否则自动切到点阵显示状态图标, 不会一片空白。
 *
 * 按键说明:
 *   K1 / K2   当前选中阈值 +1 档 / -1 档
 *   K3 短按   翻页(实时 -> 阈值 -> 自检)
 *   K3 长按   切换"正在调充满电流"还是"正在调过流保护"
 *   K4 短按   复位(清充电计时 / 解除跳闸)
 *   按住任意键 -> 板载 LED 亮对应格数(K1=1格 ... K4=4格), 松手保持 1 秒,
 *               长按被识别时这根条闪烁 —— 不用屏幕也能确认按键生效。
 *
 * 开机自检:
 *   继电器 "咔哒" 两下 -> LCD 探测 -> 板载 LED 流水灯 ->
 *   INA219 在线: 亮 4 格停一下; 不在线: 整排闪 3 次
 * 点阵图标:  √ = 一切正常    ! 闪 = INA219 掉线    × 闪 = 正在保护/跳闸
 *
 * ★ 默认阈值:
 *     充满判定 FULL = 20mA    过流保护 OVER = 100mA
 *   使用原则:  FULL 阈值 < 实际充电电流 < OVER 阈值。
 *   一上电就自动"充满"断电 -> FULL 阈值调小; 一上电就跳闸 -> OVER 阈值调大。
 * ========================================================================== */

/****************************************
 *              可设置参数
 ****************************************/
/* DS18B20 硬件故障(引脚松, 接触不良), 暂时关闭真实测温。
 * 相关代码全部保留, 传感器修好后把这行改成 1 即可恢复。 */
#define TEMP_SENSOR_EN       0

#define TEMP_LIMIT_DEFAULT   45       /* 温度上限, 单位 ℃ */
#define TEMP_LIMIT_MIN       20
#define TEMP_LIMIT_MAX       80
#define TEMP_HYST            2        /* 过温恢复要等温度降回 (上限-2)度 */

#define CUT_MA_DEFAULT       20       /* 充满判定电流, 单位 mA
                                       * 一定要比实际充电电流小! 否则一上电就会
                                       * "电流 < 阈值" 而误判充满断电 */
#define CUT_MA_MIN           10
#define CUT_MA_MAX           500
#define CUT_MA_STEP          10

#define OC_MA_DEFAULT        100      /* 过流保护阈值, 单位 mA
                                       * 要比实际充电电流大, 否则一上电就跳闸 */
#define OC_MA_MIN            20
#define OC_MA_MAX            500
#define OC_MA_STEP           20       /* 20mA 一档, 从 100 降到 20 只需按 4 下 */
#define OC_CONFIRM_CNT       5        /* 超流持续 5 * 200ms = 1 秒才跳闸, 滤掉插拔瞬间尖峰 */

#define V_ON_MV              3000     /* 总线电压高于 3V 认为接入了负载 */
#define FULL_CONFIRM_SEC     5        /* 电流持续低于阈值 5 秒才判定充满 */

/****************************************
 *              运行状态
 ****************************************/
#define ST_IDLE       0               /* 空载 */
#define ST_CHARGING   1               /* 正在充电 */
#define ST_FULL       2               /* 充满, 已自动断电 */
#define ST_OVERHEAT   3               /* 过温保护, 已断电 */
#define ST_OCURRENT   4               /* 过流跳闸, 已断电, 需按 K4 复位 */

/* ---- 可设置参数 ---- */
unsigned char g_limit = TEMP_LIMIT_DEFAULT;    /* 温度上限 ℃ */
unsigned int  g_cut   = CUT_MA_DEFAULT;        /* 充满判定电流 mA */
unsigned int  g_oc    = OC_MA_DEFAULT;         /* 过流保护阈值 mA */

/* ---- 传感器数据 ---- */
int  g_t_x10  = 0;                /* 温度, 单位 0.1℃ */
bit  g_t_ok   = 0;                /* 本次温度数据是否可用 */
bit  g_ina_ok = 0;                /* 本次电流电压数据是否可用 */
unsigned int g_mv = 0;            /* 总线电压 mV */
int  g_ma = 0;                    /* 电流 mA */

/* ---- 硬件自检结果 ---- */
bit  g_hw_t_ok   = 0;             /* DS18B20 是否在线 */
bit  g_hw_ina_ok = 0;             /* INA219 是否应答 */

/* ---- 运行状态 ---- */
unsigned char g_state    = ST_IDLE;
unsigned int  g_sec      = 0;     /* 已充电时间(秒) */
unsigned char g_full_cnt = 0;     /* 低电流持续计数, 单位 200ms */
unsigned char g_oc_cnt   = 0;     /* 过电流持续计数, 单位 200ms */
bit  g_charged = 0;               /* 本次充电是否真的测到过充电电流
                                   * 防止"负载没串进测量回路 / 电流读数恒 0"时
                                   * 一上电就误判成充满 */

/* ---- 界面 ---- */
unsigned char g_page      = 0;    /* 0=实时 1=阈值 2=自检 */
unsigned char g_page_hold = 0;    /* 临时页停留时间, 单位 500ms */
unsigned char g_sel       = 0;    /* 0=调充满电流 1=调过流保护 */
unsigned char g_disp      = 0;    /* 0=LCD1602  1=点阵 */
unsigned char g_blink     = 0;    /* 闪烁标志, 每 250ms 翻转 */

/* ---- 按键反馈条 ----
 * 按住任意键 -> 板载 LED 亮对应格数(K1=1格 ... K4=4格), 松手保持 1 秒;
 * 长按被识别时这根条闪烁。不用屏幕也能确认按键生效。 */
unsigned char g_kbar   = 0;       /* 要显示的键号 1~4, 0 = 没在按键 */
unsigned int  t_kbar   = 0;       /* 反馈开始计时 */
bit           g_kflash = 0;       /* 长按已被识别 -> 反馈条闪烁 */

/****************************************
 *            点阵显示图案
 ****************************************/
unsigned char code ICON_OK[8] =
{
    0x00, 0x01, 0x02, 0x04, 0x88, 0x50, 0x20, 0x00
};

unsigned char code ICON_ERR[8] =
{
    0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81
};

unsigned char code ICON_WARN[8] =
{
    0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00
};

/* 温度功能是否算"故障": 传感器关闭(TEMP_SENSOR_EN=0)时不算,
 * 否则点阵会一直报"!", 反而盖住了真正的问题(INA219 掉线) */
#if TEMP_SENSOR_EN
  #define T_FAULT   (!g_hw_t_ok)
#else
  #define T_FAULT   (0)
#endif

/****************************************
 *         LCD 一行缓冲(17 字节)
 * 组好一整行再一次性写出去, 省 RAM 也省时间
 ****************************************/
static char idata buf[17];   /* 放 idata: 128 字节直接寻址 RAM 放不下, 挪到高 128 字节 */

/* 追加字符串 */
static unsigned char PutS(unsigned char pos, char code *s)
{
    unsigned char i = 0;

    while((s[i] != '\0') && (pos < 16))
    {
        buf[pos] = s[i];
        pos++;
        i++;
    }
    return pos;
}

/* 追加定长十进制数, 高位不够就用 pad 补(通常是空格或 '0') */
static unsigned char PutN(unsigned char pos, unsigned int v, unsigned char len, char pad)
{
    unsigned char i;
    unsigned int  p = 1;

    for(i = 1; i < len; i++) p *= 10;

    for(i = 0; i < len; i++)
    {
        if((p > v) && (pad != '0')) buf[pos] = pad;
        else                        buf[pos] = (char)(v / p % 10 + '0');
        p /= 10;
        pos++;
    }
    return pos;
}

/* 补齐到 16 字符并写一整行 */
static void Show(unsigned char line, unsigned char pos)
{
    while(pos < 16) buf[pos++] = ' ';
    buf[16] = '\0';

    LCD_ShowString(line, 1, (char *)buf);
}

static char code *StateStr(void)
{
    switch(g_state)
    {
        case ST_CHARGING: return "CHRG";
        case ST_FULL:     return "FULL";
        case ST_OVERHEAT: return "HOT!";
        case ST_OCURRENT: return "OC!!";
    }
    return "IDLE";
}

static void Page_Set(unsigned char p)
{
    g_page = p;
    if(p == 0) g_page_hold = 0;          /* 实时页不需要自动返回 */
    else       g_page_hold = 6;          /* 3 秒后自动回到实时页 */
}

/****************************************
 *            LCD 页面内容
 ****************************************/
static void Disp_Update(void)
{
    unsigned char p;

    /* ================= 第 1 行 ================= */
    if(g_page == 0)
    {
        p = 0;
        p = PutS(p, "U:");
        if(g_ina_ok)
        {
            p = PutN(p, g_mv / 1000, 1, ' ');
            buf[p++] = '.';
            p = PutN(p, (g_mv % 1000) / 100, 1, '0');
            p = PutN(p, (g_mv % 100) / 10, 1, '0');
        }
        else
        {
            p = PutS(p, "-.--");
        }
        p = PutS(p, "V I:");
        if(g_ina_ok) p = PutN(p, (unsigned int)(g_ma < 0 ? -g_ma : g_ma), 4, ' ');
        else         p = PutS(p, "----");
        p = PutS(p, "mA");

        Show(1, p);
    }
    else if(g_page == 1)
    {
        p = 0;
        p = PutS(p, g_sel ? " " : ">");      /* > 号标出正在调的是哪一个 */
        p = PutS(p, "FULL");
        p = PutN(p, g_cut, 4, ' ');
        p = PutS(p, "mA");

        Show(1, p);
    }
    else
    {
        p = 0;
        p = PutS(p, "T-SENS:");
#if TEMP_SENSOR_EN
        p = PutS(p, g_hw_t_ok ? "OK" : "FAULT");
#else
        p = PutS(p, "OFF");      /* 传感器硬件故障, 功能已关闭(代码保留) */
#endif

        Show(1, p);
    }

    /* ================= 第 2 行 ================= */
    if(g_page == 0)
    {
        p = 0;
        p = PutS(p, StateStr());
        p = PutS(p, " ");
        p = PutN(p, g_sec / 3600, 2, '0');
        buf[p++] = ':';
        p = PutN(p, g_sec / 60 % 60, 2, '0');
        buf[p++] = ':';
        p = PutN(p, g_sec % 60, 2, '0');

        Show(2, p);
    }
    else if(g_page == 1)
    {
        p = 0;
        p = PutS(p, g_sel ? ">" : " ");
        p = PutS(p, "OVER");
        p = PutN(p, g_oc, 4, ' ');
        p = PutS(p, "mA");

        Show(2, p);
    }
    else
    {
        p = 0;
        p = PutS(p, "INA:");
        p = PutS(p, g_hw_ina_ok ? "OK" : "NG");
        p = PutS(p, "  RLY:");
        p = PutS(p, Relay_GetState() ? "ON" : "OFF");

        Show(2, p);
    }
}

/****************************************
 *           点阵状态显示
 ****************************************/
static void Disp_Matrix(void)
{
    static unsigned char col = 0;
    unsigned char v;
    unsigned char icon;

    /* 保护动作最优先, 其次是 INA219 掉线(温度功能关闭不算故障) */
    if(g_state >= ST_OVERHEAT)                     icon = 1;
    else if(T_FAULT || (!g_hw_ina_ok))             icon = 2;
    else                                           icon = 0;

    if(icon == 0)      v = ICON_OK[col];
    else if(icon == 1) v = ICON_ERR[col];
    else               v = ICON_WARN[col];

    /* 告警时整屏闪烁, 一眼就能看出来 */
    if((icon != 0) && g_blink) v = 0x00;

    MatrixLED_ShowColumn(col, v);

    col++;
    if(col >= 8) col = 0;
}

/****************************************
 *           保护控制逻辑
 *           (每 200ms 调用一次)
 ****************************************/
static void Control_Update(void)
{
    bit over;

    /* ---- 1. 过温判断: 优先级最高 ---- */
    over = 0;
    if(g_t_ok && (g_t_x10 >= (int)g_limit * 10)) over = 1;

    if(over)
    {
        Relay_Set(0);
        g_state    = ST_OVERHEAT;
        g_full_cnt = 0;
        return;
    }

    /* ---- 2. 过温保护要等温度降回 (上限-2)度 才恢复, 防止临界抖动 ---- */
    if(g_state == ST_OVERHEAT)
    {
        if(g_t_ok && (g_t_x10 > ((int)g_limit - TEMP_HYST) * 10))
        {
            Relay_Set(0);
            return;
        }
    }

    /* ---- 3. 过流跳闸是自锁的: 必须按 K4 复位才能恢复, 就像家里的空气开关 ---- */
    if(g_state == ST_OCURRENT)
    {
        Relay_Set(0);
        return;
    }

    /* ---- 4. 过流检测: 电流超过阈值持续 1 秒 -> 跳闸 ---- */
    if(g_ina_ok && (g_ma > (int)g_oc))
    {
        if(g_oc_cnt < 250) g_oc_cnt++;
        if(g_oc_cnt >= OC_CONFIRM_CNT)
        {
            Relay_Set(0);
            g_state    = ST_OCURRENT;
            g_full_cnt = 0;
            return;
        }
    }
    else
    {
        g_oc_cnt = 0;
    }

    /* ---- 5. 电流/电压判断(INA219 正常时才有效) ---- */
    if(g_ina_ok)
    {
        if(g_mv < V_ON_MV)              /* 没接负载, 断开更安全 */
        {
            Relay_Set(0);
            g_state    = ST_IDLE;
            g_full_cnt = 0;
            return;
        }

        if(g_ma >= (int)g_cut)          /* 测到真实的充电电流 -> 记下来 */
        {
            g_charged  = 1;
            g_full_cnt = 0;
        }
        else if(g_charged)              /* 电流跌下来, 且这次确实充过电 */
        {
            if(g_full_cnt < 250) g_full_cnt++;
            if(g_full_cnt >= (unsigned char)(FULL_CONFIRM_SEC * 5))
            {
                Relay_Set(0);
                g_state = ST_FULL;
                return;
            }
        }
        else                            /* 还没测到过充电电流: 不能判满
                                         * (否则电流读数恒 0 会一上电就误判充满) */
        {
            g_full_cnt = 0;
        }
    }

    /* ---- 6. 一切正常, 允许充电 ---- */
    if((g_state != ST_CHARGING) && (g_state != ST_OVERHEAT))
    {
        g_sec = 0;                      /* 新一次充电, 计时重新开始 */
    }
    Relay_Set(1);
    g_state = ST_CHARGING;
}

/****************************************
 *             按键处理
 ****************************************/
static void Reset_All(void)
{
    g_sec      = 0;
    g_full_cnt = 0;
    g_oc_cnt   = 0;
    g_charged  = 0;
    g_state    = ST_IDLE;
    Page_Set(0);
}

static void Key_Handle(unsigned char ev)
{
    unsigned char n;

    if(ev == 0) return;

    n = (unsigned char)(ev & 0x0F);

    switch(n)
    {
        case 1:
        case 2:
            if(ev & KEY_LONG_FLAG) break;

            if(g_sel == 0)              /* 调充满判定电流 */
            {
                if(n == 1)
                {
                    if(g_cut <= (CUT_MA_MAX - CUT_MA_STEP)) g_cut += CUT_MA_STEP;
                }
                else
                {
                    if(g_cut >= (CUT_MA_MIN + CUT_MA_STEP)) g_cut -= CUT_MA_STEP;
                }
            }
            else                        /* 调过流保护阈值 */
            {
                if(n == 1)
                {
                    if(g_oc <= (OC_MA_MAX - OC_MA_STEP)) g_oc += OC_MA_STEP;
                }
                else
                {
                    if(g_oc >= (OC_MA_MIN + OC_MA_STEP)) g_oc -= OC_MA_STEP;
                }
            }

            Page_Set(1);                /* 调参数时自动切到设置页 */
            break;

        case 3:
            if(ev & KEY_LONG_FLAG)      /* 长按: 切换正在调哪个参数 */
            {
                if(g_sel == 0) g_sel = 1;
                else           g_sel = 0;
                Page_Set(1);
            }
            else                        /* 短按: 翻页 */
            {
                Page_Set((unsigned char)((g_page + 1) % 3));
            }
            break;

        case 4:
            Reset_All();                /* 短按/长按都是复位 */
            break;

        default: break;
    }
}

/****************************************
 *              主函数
 ****************************************/
void main(void)
{
    unsigned int now;
    unsigned int t_key = 0, t_sensor = 0, t_ina = 0, t_ctrl = 0;
    unsigned int t_disp = 0, t_sec = 0, t_blink = 0, t_reinit = 0, t_page = 0;

    unsigned int v;
    int i;
    unsigned char ev;
    unsigned char kd;

    /* ---------- 1. 基础初始化 ---------- */
    Relay_Init();
    Timer0_Init();                      /* 1ms 心跳 */

    Relay_SelfTest();                   /* 继电器自检: "咔哒"两下 */

    /* 先吸合继电器再探测显示屏: 如果 LCD 的电是经过继电器/INA219 过来的,
     * 断电状态下它无法应答; LCD 只当显示器时这一步没有任何影响。 */
    Relay_Set(1);

    /* ---------- 2. 显示屏自检: LCD 不应答就自动改用点阵 ---------- */
    LCD_Init();
    if(LCD_Check())
    {
        g_disp = 0;
    }
    else
    {
        g_disp = 1;
        MatrixLED_Init();
    }

    /* ---------- 2.5 板载 LED 自检 ----------
     * 特意放在屏幕探测之后: LED 与 LCD 控制线共用 P2.5~P2.7,
     * 否则探测屏幕时的电平跳变会把 LED 带亮 */
    LED_Init();

    /* ---------- 3. 传感器自检 ---------- */
#if TEMP_SENSOR_EN
    g_hw_t_ok = DS18B20_Check();
    DS18B20_Start();
#else
    g_hw_t_ok = 0;                      /* 传感器硬件故障, 功能关闭(代码保留) */
#endif
    INA219_Init();
    g_hw_ina_ok = INA219_Check();

    /* ---------- 3.5 用板载 LED 报告开机状态 ----------
     * INA219 在线 -> 亮 4 格停一下
     * 不在线      -> 整排闪 3 次
     */
    if(g_hw_ina_ok)
    {
        LED_Bar(4);
        Delay(800);
        LED_Bar(0);
    }
    else
    {
        for(i = 0; i < 3; i++)
        {
            LED_Bar(LED_BAR_N);
            Delay(200);
            LED_Bar(0);
            Delay(200);
        }
    }

    /* ---------- 4. 开机画面 ---------- */
    if(g_disp == 0)
    {
        LCD_ShowString(1, 1, "CHARGE PROTECTOR");
        LCD_ShowString(2, 1, "SELF TEST...");
        Delay(1200);
    }

    g_page      = 2;                    /* 先看一眼自检结果, 再回到实时页 */
    g_page_hold = 5;                    /* 约 2.5 秒 */
    t_page      = Timer0_GetTick();

    /* ---------- 5. 主循环(全程非阻塞) ---------- */
    while(1)
    {
        now = Timer0_GetTick();

        /* 按键扫描: 每 20ms 一次 */
        if((now - t_key) >= 20)
        {
            t_key = now;
            Key_Update();
        }
        ev = Key_GetEvent();
        if(ev)
        {
            if(ev & KEY_LONG_FLAG) g_kflash = 1;      /* 长按被识别 -> 反馈条闪烁 */
            g_kbar = (unsigned char)(ev & 0x0F);
            t_kbar = now;
        }
        Key_Handle(ev);

        /* 按键反馈: 按住时亮对应格数, 松手后再保持 1 秒 */
        kd = Key_GetDown();
        if(kd)
        {
            unsigned char n = 1;

            while((kd & 0x01) == 0) { kd >>= 1; n++; }
            g_kbar = n;
            t_kbar = now;
        }
        else if((now - t_kbar) >= 1000)
        {
            g_kbar   = 0;
            g_kflash = 0;
        }

        /* ---- 传感器采样 ---- */
#if TEMP_SENSOR_EN
        if((now - t_sensor) >= 1000)
        {
            int t;

            t_sensor = now;

            /* 先取走上一次转换结果, 再启动下一次转换, 全程不用死等 */
            if(DS18B20_ReadTemp_x10(&t))
            {
                g_t_x10   = t;
                g_t_ok    = 1;
                g_hw_t_ok = 1;
            }
            else
            {
                g_t_ok    = 0;
                g_hw_t_ok = 0;
            }

            DS18B20_Start();
        }
#endif /* TEMP_SENSOR_EN */

        if((now - t_ina) >= 200)
        {
            t_ina = now;

            if(INA219_ReadAll(&v, &i))
            {
                g_mv        = v;
                g_ma        = i;
                g_ina_ok    = 1;
                g_hw_ina_ok = 1;
            }
            else
            {
                g_mv        = 0;
                g_ma        = 0;
                g_ina_ok    = 0;
                g_hw_ina_ok = 0;
            }
        }

        /* INA219 掉线后每 2 秒重试初始化(虚焊/插头松了可能又好) */
        if((!g_ina_ok) && ((now - t_reinit) >= 2000))
        {
            t_reinit = now;
            INA219_Init();
        }

        /* 保护逻辑: 每 200ms */
        if((now - t_ctrl) >= 200)
        {
            t_ctrl = now;
            Control_Update();
        }

        /* 充电计时: 每秒加一 */
        if((now - t_sec) >= 1000)
        {
            t_sec = now;
            if(g_state == ST_CHARGING) g_sec++;
        }

        /* 临时页面自动返回 */
        if(g_page_hold > 0)
        {
            if((now - t_page) >= 500)
            {
                t_page = now;
                g_page_hold--;
                if(g_page_hold == 0) g_page = 0;
            }
        }

        /* 闪烁标志 + 板载 LED 刷新 */
        if((now - t_blink) >= 250)
        {
            unsigned char lvl;

            t_blink = now;
            if(g_blink) g_blink = 0;
            else        g_blink = 1;

            /* 电流柱状条: 以过流阈值为满格, 直观显示 INA219 测到的电流 */
            lvl = 0;
            if(g_ma > 0)
            {
                lvl = (unsigned char)(((unsigned int)g_ma * 8) / g_oc);
                if(lvl > 8) lvl = 8;
            }

            if(g_kbar)                  /* 正在按键: 优先显示按键反馈条 */
            {
                if(g_kflash && g_blink) LED_Bar(0);     /* 长按已触发: 闪 */
                else                    LED_Bar(g_kbar);
            }
            else
            {
                LED_Show(g_state, g_blink, lvl);
            }
        }

        /* 显示刷新: 每 250ms */
        if(g_disp == 0)
        {
            if((now - t_disp) >= 250)
            {
                t_disp = now;
                Disp_Update();
            }
        }
        else
        {
            Disp_Matrix();
        }
    }
}
