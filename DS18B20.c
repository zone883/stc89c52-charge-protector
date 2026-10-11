#include <REGX52.H>
#include "OneWire.h"
#include "DS18B20.h"

/* DS18B20 指令 */
#define DS18B20_SKIP_ROM         0xCC
#define DS18B20_CONVERT_T        0x44
#define DS18B20_READ_SCRATCHPAD  0xBE

/* DS18B20 的物理量程 -55 ~ +125 ℃, 超出这个范围说明读数不可信 */
#define TEMP_MIN_X10  (-550)
#define TEMP_MAX_X10  (1250)

/* 传感器在线检测: 单总线复位后器件会把总线拉低(存在脉冲) */
bit DS18B20_Check(void)
{
    if(OneWire_Init() == 0) return 1;     /* 0 = 有器件应答 */
    return 0;
}

/* 启动一次温度转换(不等待, 750ms 后再来读) */
void DS18B20_Start(void)
{
    OneWire_Init();
    OneWire_SendByte(DS18B20_SKIP_ROM);
    OneWire_SendByte(DS18B20_CONVERT_T);
}

/* 读温度: 12 位分辨率, 原始值每 1 个 LSB = 0.0625℃
 * 这里先把原始值换成 0.1℃ 单位的整数, 避免在单片机里跑浮点运算
 * t 的单位是 0.1℃, 例如 t = 256 表示 25.6℃ */
bit DS18B20_ReadTemp_x10(int *t)
{
    unsigned char TLSB, TMSB;
    int  raw;
    long tmp;

    /* 读之前再确认一次器件还在, 拔掉传感器时立刻能发现 */
    if(OneWire_Init() != 0)
        return 0;

    OneWire_SendByte(DS18B20_SKIP_ROM);
    OneWire_SendByte(DS18B20_READ_SCRATCHPAD);

    TLSB = OneWire_ReceiveByte();
    TMSB = OneWire_ReceiveByte();

    /* 高字节在前, 拼成 16 位有符号数(低温时 bit15~bit11 全 1) */
    raw = (int)(((unsigned int)TMSB << 8) | TLSB);

    tmp = (long)raw * 10;         /* 0.0625℃ -> 0.1℃ */
    tmp = tmp / 16;

    /* 超量程 / 器件掉线时的乱码(比如 85℃ 上电默认值以外的怪值)一律判无效 */
    if((tmp <= TEMP_MIN_X10) || (tmp >= TEMP_MAX_X10))
        return 0;

    *t = (int)tmp;
    return 1;
}
