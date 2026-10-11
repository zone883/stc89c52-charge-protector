#include <REGX52.H>
#include "Relay.h"

sbit Relay=P1^0;


void Relay_Init(void)
{
    Relay=1;    //上电默认关闭
}


void Relay_ON(void)
{
    Relay=1;
}


void Relay_OFF(void)
{
    Relay=0;
}