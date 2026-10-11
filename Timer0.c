#include <REGX52.H>
void Timer0_Init(void)		//1??@11.0592MHz
{
	TMOD &= 0xF0;			//???????
	TL0 = 0xF5;				//???????
	TH0 = 0xFF;				//???????
	TF0 = 0;				//??TF0??
	TR0 = 1;	
	ET0=1;
	EA=1;
	PT0=0;		

}


/*void main (){
	Timer0_Init();
	while(1){
	
	P2_3=~P2_3;
		
	
	}

	}
	void Timer0() interrupt 1
	{
		static unsigned int count;
	TL0 = 0xF5;				//???????
	TH0 = 0xFF;				//???????
		
		count++;
		if(count>=10000)
		{
			count=0;
			P2_6=~P2_6;
		}
		
	}*/
		