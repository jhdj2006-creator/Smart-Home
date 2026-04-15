#include "stm32f10x.h"                  // Device header
#include "AD.h"
#include "DHT11.h"
#include "Key.h"
#include "menu.h"
#include "OLED.h"
#include "Serial.h"
#include "Store.h"
#include "Timer.h"
#include "W25Q128_SPI1.h"

int main(void)
{
	/*程序初始化*/
	AD_Init();			//光照度模块
	DHT11_Init();		//温湿度传感器
	Key_Init();			//按键
	OLED_Init();		//OLED屏
	Store_Init();		//flash存储模块
	Timer_Init();		//定时器
	Serial_Init();		//串口模块
	W25Q128_Init();		//存储模块
	
/*对传感器最大值和最小值赋值*/
//	Store_Data[2] = 23;
//	Store_Data[4] = 65;
//	Store_Data[6] = 150;
//	Store_Save();
	
	while (1)
	{
		main_menu1();//进入界面一
	}
}
