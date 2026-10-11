#include <REGX52.H>

#include "Delay.h"
#include "MatrixLED.h"
#include "DS18B20.h"
#include "INA219.h"
#include "Relay.h"
#include "Key.h"



/************** 参数 **************/

unsigned char TemperatureLimit = 30;


/************** 状态变量 **************/

float Temperature;

int Current;

unsigned char KeyNum;


/************** 点阵图案 **************/


// 正常 √

unsigned char code OK[8]=
{
    0x00,
    0x01,
    0x02,
    0x04,
    0x88,
    0x50,
    0x20,
    0x00
};


// 保护 ×

unsigned char code ERROR[8]=
{
    0x81,
    0x42,
    0x24,
    0x18,
    0x18,
    0x24,
    0x42,
    0x81
};



unsigned char DisplayBuffer[8];



/************** 点阵刷新 **************/

void Matrix_Set(unsigned char *Data)
{
    unsigned char i;

    for(i=0;i<8;i++)
    {
        DisplayBuffer[i]=Data[i];
    }
}



void Matrix_Refresh(void)
{
    static unsigned char Column=0;


    MatrixLED_ShowColumn(
        Column,
        DisplayBuffer[Column]
    );


    Column++;

    if(Column>=8)
    {
        Column=0;
    }

}





/************** 主函数 **************/

void main()
{

    unsigned int Count=0;


    /******** 初始化 ********/


    MatrixLED_Init();


    INA219_Init();


    Relay_ON();


    Matrix_Set(OK);



    while(1)
    {



        /*
        1. 点阵最高优先级
        */

        Matrix_Refresh();




        /*
        2. 按键扫描
        */

        KeyNum=Key();


        if(KeyNum==1)
        {
            TemperatureLimit++;
        }


        if(KeyNum==2)
        {
            if(TemperatureLimit>5)
            {
                TemperatureLimit--;
            }
        }





        /*
        3. 定时任务
        */

        Count++;



        if(Count>=500)
        {

            Count=0;



            /*
            DS18B20
            */

            DS18B20_ConvertT();

            Delay(750);

            Temperature=DS18B20_ReadT();




            /*
            INA219
            */

            Current=INA219_ReadCurrent();




            /*
            保护判断

            温度过高
            或
            电流异常

            */

/*
温度保护*/

if(Temperature>=TemperatureLimit)
{

    Relay_OFF();

    Matrix_Set(ERROR);

}




//保留INA219电流检测

if(Current!=0)
{

    Relay_ON();

    Matrix_Set(OK);

}

else
{

    Relay_OFF();

    Matrix_Set(ERROR);

}}}}