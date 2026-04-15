#include "stm32f10x.h"                  // Device header
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Delay.h"
#include "DHT11.h"
#include "Key.h"
#include "menu.h"
#include "OLED.h"
#include "W25Q128_SPI1.h"

/*****************************字库测试*****************************/
void W25Q128Flash(void)
{
	int8_t p;
	OLED_Clear();
	
	uint8_t SChinese[24];
	uint32_t Addr_offset = 0;//汉字的偏移地址
    uint16_t dld = 0;
	
	while(1)
	{
		OLED_Clear();
		for(u8 i = 0; i < 4; i++)
		{
			for(u8 o = 0; o < 10; o++)
			{
				W25Q128_ReadData( Addr_offset , SChinese , 24 );			
				OLED_ShowImage(o*12	 ,i*12 ,12 ,12 ,SChinese);
				OLED_UpdateArea(o*12 ,i*12 ,12 ,12);  
				Addr_offset += 24;
				OLED_ShowHexNum(0 ,48 ,Addr_offset ,6 ,OLED_6X8);
				OLED_ShowNum(0 ,56 ,Addr_offset ,8 ,OLED_6X8);
				OLED_UpdateArea(0 ,48 ,48 ,16); 
				Delay_ms(dld);			
			}
		}		
		
		p = menu_Roll_event();   
		if( p == 1){ 
			dld += 10;
		}	 
		else if( p == -1){ 
		   if(dld) dld -= 10;
		}	
		p=menu_Enter_event();
		if(p == 1){
			return;
		}
		else if ( p == 2 ){
			return;
		}	
	}
}

/*****************************屏幕亮度*****************************/
uint8_t liangdu = 0xFF;

void particulars(void)
{
		OLED_WriteCommand(0x81);	//设置对比度
		OLED_WriteCommand(liangdu);	//0x00~0xFF
}

/*****************************光标样式*****************************/
extern uint8_t Cursor;

void System_Cursor1(void){Cursor = 0;}
void System_Cursor2(void){Cursor = 1;}
void System_Cursor3(void){Cursor = 2;}

void System_Cursor(void)
{
	struct option_class2 option_list[] = 
	{
		{"- 返回",1, 0},
		{"- 指针",1, System_Cursor1},		//{选项名，需要运行的函数}
		{"- 矩形",1, System_Cursor2},
		{"- 反色",1, System_Cursor3},
		{".."}		//结尾标志,方便计算数量
	};
	run_menu2(option_list);
}

/*****************************屏幕反色*****************************/
uint8_t fanse = 0 ;

void OLEDfanse(void)
{
	if(fanse)
	{
		OLED_WriteCommand(0xA7);
	}
	else
	{
		OLED_WriteCommand(0xA6);
	}
}

/*****************************菜单子项*****************************/
uint8_t cursor;

void System_information(void)	//信息
{
	struct option_class2 option_list[] = {
		{"- 返回",Function,0},
		{"-----STM32C8T6-----"	,Display},		//{选项名，需要运行的函数}
		{"- RAM: 20K"			,Display},
		{"- FLASH: 64K"			,Display},
		{"------W25Q128------"	,Display},
		{"- 储存:16MB"			,Display},
		{"-------------------"	,Display},
		{"- 软件版本V1.15"		,Display},
		{".."}		//结尾标志,方便计算数量
	};
	cursor = Cursor;
	Cursor = 2;
	run_menu2(option_list);
	Cursor = cursor;
}

void System_settings(void)		//设置
{
	struct option_class2 option_list[] = {
		{"- 返回"		,Function	, 0},
		{"- 屏幕亮度"	,NumBer		, particulars	, &liangdu},		//{选项名，需要运行的函数}
		{"- 光标样式"	,Function	, System_Cursor},
		{"- 反色显示"	,ON_OFF		, OLEDfanse		, &fanse},
		{"- 信息"		,Function	, System_information},
		{".."}		//结尾标志,方便计算数量
	};
	run_menu2(option_list);
}

extern uint8_t hum;			//湿度
extern uint8_t temp;		//温度
extern uint8_t light;		//光照度
extern uint8_t sen_sj;

void Sensor_temp(void)	{sen_sj = 0;}
void Sensor_hum(void)	{sen_sj = 1;}
void Sensor_light(void)	{sen_sj = 2;}

void System_sensor(void)		//温湿光照
{
	struct option_class2 option_sensor[] = {
		{"- 返回"		,Function	, 0},
		{"- 设置:"		,Sensor		, Sensor_temp	, &temp},			//界面三用界面二的代码！！！！！！！！！！！！！！！！！
		{"- 温度:"		,Sensor		, Sensor_temp	, &temp},			//{选项名，需要运行的函数}
		{"- 湿度:"		,Sensor		, Sensor_hum	, &hum},
		{"- 光照度:"		,Sensor		, Sensor_light	, &light},
		{".."}		//结尾标志,方便计算数量
	};
	
	run_menu2(option_sensor);
}

extern uint8_t dianji1;
extern uint8_t act_sj;

void Actuator_dianji1(void)	{act_sj = 0;}

void System_actuator(void)		//电机
{
	struct option_class2 option_sensor[] = {
		{"- 返回"		,Function	, 0},
		{"- 电机1:"		,Actuator	, Actuator_dianji1	, &dianji1},	//{选项名，需要运行的函数}
		{".."}		//结尾标志,方便计算数量
	};
	
	run_menu2(option_sensor);
}

