#include "stm32f10x.h"		// Device header
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "AD.h"
#include "Delay.h"
#include "DHT11.h"
#include "Key.h"
#include "menu.h"
#include "OLED.h"
#include "Store.h"
#include "Serial.h"

void error(void)
{
	OLED_Clear();
	OLED_ShowString(0, 16, "风扇", OLED_8X16);
	OLED_Update();
}

uint8_t  Up, Down, Back, Enter;

static uint8_t initialDelay = 15;			// 短按初值 d1
static uint8_t repeatDelay = 1;				// 长按初值 d2

static bool isUpPressed = false;			// 用来判断按键状态（是否按下） up
static uint8_t upCounter = 0;				// 用来判断是否为长按或短按
static uint8_t currentDelayUp = 15;			// up间隔时间

static bool isDownPressed = false;			// 用来判断按键状态（是否按下） down
static uint8_t downCounter = 0;				// 用来判断是否为长按或短按
static uint8_t currentDelayDown = 15;		// down间隔时间

static bool isPressed = false;
static bool isLongPressTriggered = false;
static uint8_t pressCounter = 0;

/******菜单滚动****************************************************/
int8_t menu_Roll_event(void)
{
	if (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0) == 0) {	//按键按下
		if(!isUpPressed) {//短按滚动
			//首次按下：立即触发，初始化计算器
			isUpPressed = true;
			upCounter = 0;
			currentDelayUp = initialDelay;
			return 1;
		}else {//长按滚动	//延迟15次循环判断是否为长按 超过15次循环则进入
			if (++upCounter >= currentDelayUp) {//初始为15次，进入后为1次
				upCounter = 0;
				currentDelayUp = repeatDelay;
				return 1;
			}
		}
    }else {
        isUpPressed = false;//按键释放后复位状态
    }
	
	if (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1) == 0) {
		if (!isDownPressed) {
			isDownPressed = true;
            downCounter = 0;
            currentDelayDown = initialDelay;
            return -1;
        } else {
			if (++downCounter >= currentDelayDown) {
				downCounter = 0;
				currentDelayDown = repeatDelay;
				return -1;
			}
        }
    } else {
        isDownPressed = false;
    }
	return 0;//无按键状态
}

/******菜单确认****************************************************/
int8_t menu_Enter_event(void)
{
    if (GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_13) == 0) {
        if (!isPressed) {
            // 首次按下：立即触发，初始化计算器
            isPressed = true;
            pressCounter = 0;
            isLongPressTriggered = false;
        } else {
            // 延迟15次循环	判断是否为长按 超过15次循环则进入
            if (++pressCounter >= initialDelay && !isLongPressTriggered) {
                isLongPressTriggered = true;
                return 1; // 长按确认
            }
        }
        return 0;
    } else {
        // 按键释放处理
        if (isPressed) {
            isPressed = false;
            if (isLongPressTriggered) {
                isLongPressTriggered = false;
                return 0; // 长按已触发后不返回短按
            } else {
                return 1; // 短按确认
            }
        }
        return 0;
    }
}

/******菜单返回****************************************************/
int8_t menu_Back_event(void)
{
	if(Back)
	{
		Back=0;
		return 1;
	}else 
		return 0;
}

/************************************************************************************************************************************************/

int8_t Speed_Factor = 12;		//光标动画速度系数;
float  Roll_Speed = 1.3;		//滚动动画速度系数;

void main_menu1(void)//界面编辑（更改界面在这个地方）
{
	struct option_class1 option_list[] =
	{
		/*中文显示			, 代码段				, 图标数组		*/
		{"W25Q128字库测试"	, W25Q128Flash		, bug			},
		{"设置"				, System_settings	, APPSET		},	//重新写三级菜单，逻辑用二级菜单！！！！！！！！！！！
		{"传感器"			, System_sensor		, APPSENSOR	 	},
		{"执行器"			, System_actuator	, APPACTUATOR	},
		{"蓝牙"				, error				, APPBLUETOOTH	},
		{"返回"				, RETURN			, APPBIPAN		},
		{".."}	//结尾标志,方便计算数量
		
	};
	run_menu1(option_list);//加载界面（在OLED屏上显示出来）
}

/************************************************************************************************************************************************/
uint8_t Cursor = 0;
uint8_t hum;		//湿度
uint8_t temp;		//温度
uint8_t light;		//光照度
uint8_t sen_sj;

uint8_t dianji1;
uint8_t RxData2;
uint8_t Serial2_RxFlag;
uint8_t act_sj;

void run_menu2(struct option_class2* option)
{
	int8_t  Catch_i = 1;		   		//选中下标
	int8_t  Cursor_i = 0;	      		//光标下标
	int8_t  Show_i = 0; 		 		//显示起始下标
	int8_t  Max = 0;					//选项数量
	int8_t  Roll_Event = 0;	   			//按键事件
	uint8_t Prdl;            			//进度条单格长度
	float  mubiao = 0 , mubiao_up = 0;  //目标进度条长度，上次进度条长度
	while(option[++Max].Name[0] != '.');//获取条目数量,如果文件名开头为'.'则为结尾;
	Max--;								//不打印".."
	
	for(int8_t i = 0; i <= Max; i++)	//计算选项名宽度;
	{		
		option[i].NameLen = Get_NameLen(option[i].Name);
	}
	
	static float Cursor_len_d0 = 0, Cursor_len_d1 = 0, Cursor_i_d0 = 0, Cursor_i_d1 = 0;		//光标位置和长度的起点终点
	
	int8_t Show_d = 0, Show_i_temp = Max;		//显示动画相关
	
	Prdl = 64 / Max;		// 屏幕高度 / 条目数量
	
	while(1)
	{
		OLED_Clear();
		Roll_Event = menu_Roll_event();				//获取按键事件
		if(Roll_Event)								//如果有按键事件
		{
			Cursor_i += Roll_Event;					//更新下标
			Catch_i += Roll_Event;
			
			if(Catch_i < 0) 	{Catch_i = 0;}		//限制选中下标
			if(Catch_i > Max) 	{Catch_i = Max;}
			
			if(Cursor_i < 0) 	{Cursor_i = 0;}		//限制光标位置
			if(Cursor_i > 3) 	{Cursor_i = 3;}
			if(Cursor_i > Max) 	{Cursor_i = Max;}
		}
		
/*****************************显示相关*****************************/
		Show_i = Catch_i - Cursor_i;				//计算显示起始下标
		
		if(Show_i - Show_i_temp)					//如果下标有偏移
		{
			Show_d = (Show_i - Show_i_temp) * MEHE;
			Show_i_temp = Show_i;
		}
		if(Show_d) {Show_d /= Roll_Speed;}			//滚动变化速度
		
		for(int8_t i = 0; i < 5; i++)				//遍历显示选项名和对应参数
		{	
			if(Show_i + i > Max ) {break;}
			OLED_ShowString(2, (i* MEHE)+Show_d +2, option[Show_i + i].Name, OLED_6X8);
			
			if( option[Show_i + i].mode == ON_OFF )			//如果是开关型
			{
				OLED_DrawRectangle(96, (i* MEHE)+Show_d +2, 12, 12, OLED_UNFILLED);
				if( *( option[Show_i + i].Num) )	//如果开关打开(传入值为真)
				{
					OLED_DrawRectangle(99, (i* MEHE)+Show_d +5, 6, 6, OLED_FILLED);
				}
			}
			else if( option[Show_i + i].mode == NumBer )	//如果是设置型
			{
				//！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
			}
			else if( option[Show_i + i].mode == NumBer )	//如果是变量型
			{
				//如果百位不为零
				if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 0 - 1) % 10 )
				OLED_ShowNum(92, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 3, OLED_6X8);
				//如果十位不为零
				else if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 1 - 1) % 10 )
				OLED_ShowNum(95, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 2, OLED_6X8);
				//如果个位不为零
				else if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 2 - 1) % 10 )
				OLED_ShowNum(98, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 1, OLED_6X8);
				else
				OLED_ShowNum(98, (i* MEHE)+Show_d + 6, 0, 1, OLED_6X8);
			}
			else if( option[Show_i + i].mode == Sensor )	//如果是传感器
			{
				//如果百位不为零
				if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 0 - 1) % 10 )
				OLED_ShowNum(85, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 3, OLED_6X8);
				//如果十位不为零
				else if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 1 - 1) % 10 )
				OLED_ShowNum(85, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 2, OLED_6X8);
				//如果个位不为零
				else if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 2 - 1) % 10 )
				OLED_ShowNum(85, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 1, OLED_6X8);
				else
				OLED_ShowNum(85, (i* MEHE)+Show_d + 6, 0, 1, OLED_6X8);
			}
			else if( option[Show_i + i].mode == Actuator )	//如果是执行器
			{
				//如果百位不为零
				if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 0 - 1) % 10 )
				OLED_ShowNum(85, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 3, OLED_6X8);
				//如果十位不为零
				else if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 1 - 1) % 10 )
				OLED_ShowNum(85, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 2, OLED_6X8);
				//如果个位不为零
				else if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 2 - 1) % 10 )
				OLED_ShowNum(85, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 1, OLED_6X8);
				else
				OLED_ShowNum(85, (i* MEHE)+Show_d + 6, 0, 1, OLED_6X8);
			}
		}
/*****************************光标相关*****************************/
		Cursor_i_d1 = Cursor_i * MEHE;							//轮询光标目标位置
		Cursor_len_d1 = option[Catch_i].NameLen * WORD_H + 15;	//轮询光标目标宽度
		
		/*计算此次循环光标位置*///如果当前位置不是目标位置,当前位置向目标位置移动
		if((Cursor_i_d1 - Cursor_i_d0) > 1) {Cursor_i_d0 += (Cursor_i_d1 - Cursor_i_d0) / Speed_Factor + 1;}
		else if((Cursor_i_d1 - Cursor_i_d0) < -1) {Cursor_i_d0 += (Cursor_i_d1 - Cursor_i_d0) / Speed_Factor - 1;}
		else {Cursor_i_d0 = Cursor_i_d1;}
		
		/*计算此次循环光标宽度*/
		if((Cursor_len_d1 - Cursor_len_d0) > 1) {Cursor_len_d0 += (Cursor_len_d1 - Cursor_len_d0) / Speed_Factor + 1;}
		else if((Cursor_len_d1 - Cursor_len_d0) < -1) {Cursor_len_d0 += (Cursor_len_d1 - Cursor_len_d0) / Speed_Factor - 1;}
		else {Cursor_len_d0 = Cursor_len_d1;}
		
		/*显示光标*/
		if(Cursor==0)		OLED_ShowString(Cursor_len_d0, Cursor_i_d0+2, "<-", OLED_6X8);	//尾巴光标
		else if(Cursor==1)	OLED_DrawRectangle(0, Cursor_i_d0, Cursor_len_d0 + 8, 18, OLED_UNFILLED);
		else if(Cursor==2)	OLED_ReverseArea2 (0, Cursor_i_d0, Cursor_len_d0, 16);			//对指定位置取反

/*****************************进度条*****************************/
		if(Catch_i == Max) {mubiao = 64 ;}
		else{ mubiao = Prdl * Catch_i ;}
		
		if(mubiao_up<mubiao) { mubiao_up += 1.4 ;}
		if(mubiao_up>mubiao) { mubiao_up -= 1.4 ;}
		
		OLED_DrawLine(123, 0,	127, 0);
		OLED_DrawLine(125, 0,	125, 63);
		OLED_DrawLine(123, 63, 	127, 63);
		OLED_DrawRectangle(123, 0, 5, mubiao_up, OLED_FILLED);
		
/*****************************刷新屏幕*****************************/
		OLED_Update();
		
/*****************************获取按键*****************************/
		if(menu_Enter_event())	//进入
		{
			if( option[Catch_i].mode == Function )		//如果是函数
			{
				/*如果功能不为空则执行功能,否则返回*/
				if(option[Catch_i].func)
				{
					option[Catch_i].func();
				}else
				{
					return;
				}
			}
			else if( option[Catch_i].mode == ON_OFF )	//如果是开关
			{
				*( option[Catch_i].Num) =  !*( option[Catch_i].Num);//当前变量取反
				if( option[Catch_i].func){option[Catch_i].func();}	//执行功能一次
			}
			else if( option[Catch_i].mode == NumBer )	//如果是变量
			{
				/*内部的变量调节器*/
				int8_t p;
				float NP = 0 , NP_target = 96;			//变量框框大小，目标框框大小
				float GP = 0 , GP_target;				//变量条条长度，目标条条长度
//				float tap = 80/255;						//条条单长0.31
				while(1)
				{
					if( NP < NP_target )				//如果变量框框没画完
					{
						if( NP_target - NP > 25 ) NP += 5 ;
						else if( NP_target - NP > 5 ) NP += 2.5 ;
						else if( NP_target - NP > 0 ) NP += 0.5 ;
						
						OLED_DrawRectangle(16, 8, NP, NP / 2, OLED_UNFILLED);
						OLED_ClearArea(17, 9, NP - 2, NP / 2 - 2);
						OLED_Update();
					}else
					{
//						GP_target = *( option[Catch_i].Num) * tap ;
						GP_target = *( option[Catch_i].Num) * 0.31 ;
						if(GP < GP_target)
						{
							if( GP_target - GP > 25 ) GP += 5 ;
							else if( GP_target - GP > 7 ) GP += 2.4 ;
							else if( GP_target - GP > 0 ) GP += 0.5 ;
						}
						if(GP > GP_target)
						{
							if( GP - GP_target > 25 ) GP -= 5 ;
							else if( GP - GP_target > 7 ) GP -= 2.4 ;
							else if( GP - GP_target > 0 ) GP -= 0.5 ;
						}
						
						OLED_ClearArea( 24, 43, 80, 3);
						OLED_DrawRectangle( 24, 43, GP , 3, OLED_FILLED);
						OLED_ClearArea( 55, 20, 18, 8);
						//如果百位不为零
						if ( *( option[Catch_i].Num) / OLED_Pow(10, 3 - 0 - 1) % 10 )
						OLED_ShowNum(55, 20, *( option[Catch_i].Num), 3, OLED_6X8);
						//如果十位不为零
						else if ( *( option[Catch_i].Num) / OLED_Pow(10, 3 - 1 - 1) % 10 )
						OLED_ShowNum(58, 20, *( option[Catch_i].Num), 2, OLED_6X8);
						//如果个位不为零
						else if ( *( option[Catch_i].Num) / OLED_Pow(10, 3 - 2 - 1) % 10 )
						OLED_ShowNum(61, 20, *( option[Catch_i].Num), 1, OLED_6X8);
						else
						OLED_ShowNum(61, 20, 0, 1, OLED_6X8);
						OLED_Update();
					}
					
					p = menu_Roll_event();//变量 按键检测
					if( p == 1)
					{
						*( option[Catch_i].Num) += 1;
						if( option[Catch_i].func){option[Catch_i].func();} //执行功能一次
					}else if( p == -1)
					{
						*( option[Catch_i].Num) -= 1;
						if( option[Catch_i].func){option[Catch_i].func();} //执行功能一次
					}
					
					p = menu_Enter_event();
					if( p == 1)
					{
						OLED_Clear();
						OLED_Update();
						break;
					}else if ( p == 1)
					{
						OLED_Clear();
						OLED_Update();
						break;
					}
				}
			}
			else if( option[Catch_i].mode == Sensor )	//如果是传感器
			{
				/*内部的变量调节器*/
				int8_t p;
				uint8_t num = 0;//汉字长度
				float NP = 0 , NP_target = 115;			//变量框框大小，目标框框大小
				float GP = 0 , GP_target = 0 , GP_max = 0 , GP_min = 0;//变量条条长度，目标条条长度，传感器数据条最大长度，传感器数据条最小长度
				
				/*传感器数据*/
				uint8_t temp_max = Store_Data[1];
				uint8_t hum_max = Store_Data[3];
				uint8_t light_max = Store_Data[5];
				
				uint8_t temp_min = Store_Data[2];
				uint8_t hum_min = Store_Data[4];
				uint8_t light_min = Store_Data[6];
				
				if(option[Catch_i].func)
				{
					option[Catch_i].func();
				}else
				{
					return;
				}
				
				while(1)
				{
					if( NP < NP_target )//如果变量框框没画完
					{
						if( NP_target - NP > 25 ) NP += 5 ;
						else if( NP_target - NP > 5 ) NP += 2.5 ;
						else if( NP_target - NP > 0 ) NP += 0.5 ;
						
						OLED_DrawRectangle(4, 2, NP, NP / 2, OLED_UNFILLED);
						OLED_ClearArea(5, 3, NP - 2, NP / 2 - 2);
						OLED_Update();
						
					}else
					{
						GP_target = *( option[Catch_i].Num) * 0.31 ;
						
						if(sen_sj == 2){
							GP_target = *( option[Catch_i].Num) * 0.81 ;
						}
						
						if(GP < GP_target)
						{
							if( GP_target - GP > 25 ) GP += 5 ;
							else if( GP_target - GP > 7 ) GP += 2.4 ;
							else if( GP_target - GP > 0 ) GP += 0.5 ;
						}
						if(GP > GP_target)
						{
							if( GP - GP_target > 25 ) GP -= 5 ;
							else if( GP - GP_target > 7 ) GP -= 2.4 ;
							else if( GP - GP_target > 0 ) GP -= 0.5 ;
						}
						
						if(sen_sj == 0)//如果是温度
						{
							num = 0;
							
							/*判断最大值*/
							if(temp > temp_max){temp_max = temp;}
							GP_max = temp_max * 0.31 ;
							
							/*判断最小值*/
							if(temp < temp_min){temp_min = temp;}
							GP_min = temp_min * 0.31 ;
							
							Store_Data[1] = temp_max;
							Store_Data[2] = temp_min;
							Store_Save();
							
							/*固定数据*/
							OLED_ShowString(5,13,"当前温度:",OLED_6X8);
							OLED_ShowString(5,1,"max:",OLED_6X8);
							OLED_ShowString(50,1,"min:",OLED_6X8);
							OLED_ShowNum(30, 5, temp_max, 2, OLED_6X8);
							OLED_ShowNum(75, 5, temp_min, 2, OLED_6X8);
							
							/*动态数据*/
							OLED_ClearArea(10, 35, 105, 8);
							OLED_ClearArea(10, 47, 105, 8);
							OLED_ShowNum(GP + 58, 35, temp, 2, OLED_6X8);//进度条当前温度
							OLED_ShowNum(GP_max + 58, 47, temp_max, 2, OLED_6X8);//显示最大值
							if(GP_max - GP_min > 13){//如果最大值与最小值的间距小于 13 则不显示
								OLED_ShowNum(GP_min + 58, 47, temp_min, 2, OLED_6X8);//显示最小值
							}
						}
						if(sen_sj == 1)//如果是湿度
						{
							num = 0;
							
							/*判断最大值*/
							if(hum > hum_max){hum_max = hum;}
							GP_max = hum_max * 0.31 ;
							
							/*判断最小值*/
							if(hum < hum_min){hum_min = hum;}
							GP_min = hum_min * 0.31 ;
							
							Store_Data[3] = hum_max;
							Store_Data[4] = hum_min;
							Store_Save();
							
							/*固定数据*/
							OLED_ShowString(5,	13,	"当前湿度:",	OLED_6X8);
							OLED_ShowString(5,	1,	"max:",		OLED_6X8);
							OLED_ShowString(50,	1,	"min:",		OLED_6X8);
							OLED_ShowNum(30, 5, hum_max, 2, OLED_6X8);
							OLED_ShowNum(75, 5, hum_min, 2, OLED_6X8);
							
							/*动态数据*/
							OLED_ClearArea(10, 35, 105, 8);
							OLED_ClearArea(10, 47, 105, 8);
							OLED_ShowNum(GP + 18, 35, hum, 2, OLED_6X8);//进度条当前湿度
							OLED_ShowNum(GP_max + 18, 47, hum_max, 2, OLED_6X8);//显示最大值
							if(GP_max - GP_min > 13){//如果最大值与最小值的间距小于 13 则不显示
								OLED_ShowNum(GP_min + 18, 47, hum_min, 2, OLED_6X8);//显示最小值
							}
						}
						if(sen_sj == 2)//如果是光照度
						{
							num = 16;
							
							/*判断最大值*/
							if(light > light_max){light_max = light;}
							GP_max = light_max * 0.81 ;
							
							/*判断最小值*/
							if(light < light_min){light_min = light;}
							GP_min = light_min * 0.81 ;
							
							Store_Data[5] = light_max;
							Store_Data[6] = light_min;
							Store_Save();
							
							/*固定数据*/
							OLED_ShowString(5,	13,	"当前光照度:", OLED_6X8);
							OLED_ShowString(5,	1,	"max:", OLED_6X8);
							OLED_ShowString(50,	1,	"min:", OLED_6X8);
							OLED_ShowNum(30, 5, light_max, 3, OLED_6X8);
							OLED_ShowNum(75, 5, light_min, 3, OLED_6X8);
							
							/*动态数据*/
							OLED_ClearArea(10, 35, 105, 8);
							OLED_ClearArea(10, 47, 105, 8);
							OLED_ShowNum(GP + 18, 35, light, 2, OLED_6X8);//进度条当前光照度
							OLED_ShowNum(GP_max + 18, 47, light_max, 2, OLED_6X8);//显示最大值
							if(GP_max - GP_min > 13){//如果最大值与最小值的间距小于 16 则不显示
								OLED_ShowNum(GP_min + 18, 47, light_min, 2, OLED_6X8);//显示最小值
							}
						}
						
						//进度条位置//
						if(sen_sj == 0){//温度
							OLED_ClearArea(24, 43, 80, 3);
							OLED_DrawLine(64, 44, GP_max + 63, 44);
							OLED_DrawRectangle(	GP_max + 62, 43, 2, 3, OLED_FILLED);
							OLED_DrawRectangle(	64, 43, GP , 3, OLED_FILLED);
						}else{//湿度、光照度
							OLED_ClearArea(24, 43, 80, 3);
							OLED_DrawLine(24, 44, GP_max + 23, 44);
							OLED_DrawRectangle(GP_max + 22, 43, 2 , 3, OLED_FILLED);
							OLED_DrawRectangle(24, 43, GP , 3, OLED_FILLED);
						}
						
						OLED_ClearArea(57 + num, 17, 25, 8);
						//如果百位不为零
						if ( *( option[Catch_i].Num) / OLED_Pow(10, 3 - 0 - 1) % 10 )
						OLED_ShowNum(59 + num, 17, *( option[Catch_i].Num), 3, OLED_6X8);
						//如果十位不为零
						else if ( *( option[Catch_i].Num) / OLED_Pow(10, 3 - 1 - 1) % 10 )
						OLED_ShowNum(62 + num, 17, *( option[Catch_i].Num), 2, OLED_6X8);
						//如果个位不为零
						else if ( *( option[Catch_i].Num) / OLED_Pow(10, 3 - 2 - 1) % 10 )
						OLED_ShowNum(65 + num, 17, *( option[Catch_i].Num), 1, OLED_6X8);
						else
						OLED_ShowNum(65 + num, 17, 0, 1, OLED_6X8);
						OLED_Update();
					}
					
					//按键检测//
					p = menu_Roll_event();
					if( p == 1)
					{
						*( option[Catch_i].Num) += 1;
						if( option[Catch_i].func){option[Catch_i].func();} //执行功能一次
					}else if( p == -1)
					{
						*( option[Catch_i].Num) -= 1;
						if( option[Catch_i].func){option[Catch_i].func();} //执行功能一次
					}
					
					p = menu_Enter_event();//返回
					if( p == 1)
					{
						while( NP > 0 )				//如果变量框框没画完	退出动画
						{
							for(int8_t i = 0; i < 5; i++)				//遍历显示选项名和对应参数
							{
								if(Show_i + i > Max ) {break;}
								OLED_ShowString(2, (i* MEHE)+Show_d +2, option[Show_i + i].Name, OLED_6X8);
								
								if( option[Show_i + i].mode == Sensor )
								{
									//如果百位不为零
									if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 0 - 1) % 10 )
									OLED_ShowNum(85, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 3, OLED_6X8);
									//如果十位不为零
									else if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 1 - 1) % 10 )
									OLED_ShowNum(85, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 2, OLED_6X8);
									//如果个位不为零
									else if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 2 - 1) % 10 )
									OLED_ShowNum(85, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 1, OLED_6X8);
									else
									OLED_ShowNum(85, (i* MEHE)+Show_d + 6, 0, 1, OLED_6X8);
								}
							}
							
							if(Cursor==0)		OLED_ShowString(Cursor_len_d0, Cursor_i_d0+2, "<-", OLED_6X8);	//尾巴光标
							else if(Cursor==1)	OLED_DrawRectangle(0, Cursor_i_d0, Cursor_len_d0 + 8, 18, OLED_UNFILLED);
							else if(Cursor==2)	OLED_ReverseArea2 (0, Cursor_i_d0, Cursor_len_d0, 16);			//对指定位置取反
							
							if( NP_target - NP > 25 ) OLED_ClearArea(4, 2, NP + 5, NP / 2) ;
							else if( NP_target - NP > 5 ) OLED_ClearArea(4, 2, NP + 5, NP / 2);
							else if( NP_target - NP >= 0 ) OLED_ClearArea(4, 2, NP + 5, NP / 2);
							
							if( NP_target - NP > 25 ) NP -= 5 ;
							else if( NP_target - NP > 5 ) NP -= 2.5 ;
							else if( NP_target - NP >= 0 ) NP -= 1 ;
							
							OLED_DrawRectangle(4, 2, NP, NP / 2, OLED_UNFILLED);
							OLED_Update();
						}
						break;
					}
				}
			}//传感器	结束
			else if( option[Catch_i].mode == Actuator )	//如果是执行器
			{
				/*内部的变量调节器*/
				int8_t p;
				float NP = 0 , NP_target = 115;			//变量框框大小，目标框框大小
				float GP = 0 , GP_target = 0 , GP_max = 80;//变量条条长度，目标条条长度，传感器数据条最大长度，传感器数据条最小长度
								
				if(option[Catch_i].func)
				{
					option[Catch_i].func();
				}
				else
				{
					return;
				}
				
				while(1)
				{
					if( NP < NP_target )//如果变量框框没画完
					{
						if( NP_target - NP > 25 ) NP += 5 ;
						else if( NP_target - NP > 5 ) NP += 2.5 ;
						else if( NP_target - NP > 0 ) NP += 0.5 ;
						
						OLED_DrawRectangle(4, 2, NP, NP / 2, OLED_UNFILLED);
						OLED_ClearArea(5, 3, NP - 2, NP / 2 - 2);
						OLED_Update();
						
					}else
					{
						GP_target = *( option[Catch_i].Num) * 0.8 ;
						
						if(GP < GP_target)
						{
							if( GP_target - GP > 25 ) GP += 5 ;
							else if( GP_target - GP > 7 ) GP += 2.4 ;
							else if( GP_target - GP > 0 ) GP += 0.5 ;
						}
						if(GP > GP_target)
						{
							if( GP - GP_target > 25 ) GP -= 5 ;
							else if( GP - GP_target > 7 ) GP -= 2.4 ;
							else if( GP - GP_target > 0 ) GP -= 0.5 ;
						}
						
						if(act_sj == 0)
						{							
							/*固定数据*/
							OLED_ShowString(5,13,"当前转速:",OLED_6X8);
							
							/*动态数据*/
							OLED_ClearArea(10, 35, 105, 8);
							OLED_ClearArea(10, 47, 105, 8);
							if(dianji1 >= 100)
							{
								OLED_ShowNum(GP + 18, 35, dianji1, 3, OLED_6X8);//进度条当前
							}else
							{
								OLED_ShowNum(GP + 18, 35, dianji1, 2, OLED_6X8);//进度条当前
							}							
						}
						
						//进度条位置//
						OLED_ClearArea(24, 43, 80, 3);
						OLED_DrawLine(24, 44, GP_max + 23, 44);
						OLED_DrawRectangle(GP_max + 22, 43, 2 , 3, OLED_FILLED);
						OLED_DrawRectangle(24, 43, GP , 3, OLED_FILLED);
						
						OLED_ClearArea(57, 17, 25, 8);
						//如果百位不为零
						if ( *( option[Catch_i].Num) / OLED_Pow(10, 3 - 0 - 1) % 10 )
						OLED_ShowNum(59, 17, *( option[Catch_i].Num), 3, OLED_6X8);
						//如果十位不为零
						else if ( *( option[Catch_i].Num) / OLED_Pow(10, 3 - 1 - 1) % 10 )
						OLED_ShowNum(62, 17, *( option[Catch_i].Num), 2, OLED_6X8);
						//如果个位不为零
						else if ( *( option[Catch_i].Num) / OLED_Pow(10, 3 - 2 - 1) % 10 )
						OLED_ShowNum(65, 17, *( option[Catch_i].Num), 1, OLED_6X8);
						else
						OLED_ShowNum(65, 17, 0, 1, OLED_6X8);
						OLED_Update();
					}
					
					//按键检测//
					p = menu_Roll_event();
					if( p == 1)
					{
						*( option[Catch_i].Num) += 20;
						if( option[Catch_i].func){option[Catch_i].func();} //执行功能一次
					}else if( p == -1)
					{
						*( option[Catch_i].Num) -= 20;
						if( option[Catch_i].func){option[Catch_i].func();} //执行功能一次
					}
					
					if(p != 0)
					{
						if(*( option[Catch_i].Num) >100){
							*( option[Catch_i].Num) = 0;
						}
						
						if(*( option[Catch_i].Num) == 0)
						{
							Serial_SendString("@DJA_");
							Serial_SendNumber(*( option[Catch_i].Num), 1);
							Serial_SendString("\r\n");
						}
						else if(*( option[Catch_i].Num) == 20)
						{
							Serial_SendString("@DJA_");
							Serial_SendNumber(*( option[Catch_i].Num), 2);
							Serial_SendString("\r\n");

						}
						else if(*( option[Catch_i].Num) == 40)
						{
							Serial_SendString("@DJA_");
							Serial_SendNumber(*( option[Catch_i].Num), 2);
							Serial_SendString("\r\n");

						}
						else if(*( option[Catch_i].Num) == 60)
						{
							Serial_SendString("@DJA_");
							Serial_SendNumber(*( option[Catch_i].Num), 2);
							Serial_SendString("\r\n");

						}
						else if(*( option[Catch_i].Num) == 80)
						{
							Serial_SendString("@DJA_");
							Serial_SendNumber(*( option[Catch_i].Num), 2);
							Serial_SendString("\r\n");

						}
						else if(*( option[Catch_i].Num) == 100)
						{
							Serial_SendString("@DJA_");
							Serial_SendNumber(*( option[Catch_i].Num), 3);
							Serial_SendString("\r\n");

						}
					}
					
					if (Serial2_RxFlag == 1){
						*( option[Catch_i].Num) = RxData2;
					}
					
					p = menu_Enter_event();//返回
					if( p == 1)
					{
						while( NP > 0 )				//如果变量框框没画完	退出动画
						{
							for(int8_t i = 0; i < 5; i++)				//遍历显示选项名和对应参数
							{
								if(Show_i + i > Max ) {break;}
								OLED_ShowString(2, (i* MEHE)+Show_d +2, option[Show_i + i].Name, OLED_6X8);
								
								if( option[Show_i + i].mode == Actuator )
								{
									//如果百位不为零
									if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 0 - 1) % 10 )
									OLED_ShowNum(85, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 3, OLED_6X8);
									//如果十位不为零
									else if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 1 - 1) % 10 )
									OLED_ShowNum(85, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 2, OLED_6X8);
									//如果个位不为零
									else if ( *( option[Show_i + i].Num) / OLED_Pow(10, 3 - 2 - 1) % 10 )
									OLED_ShowNum(85, (i* MEHE)+Show_d + 6, *( option[Show_i + i].Num), 1, OLED_6X8);
									else
									OLED_ShowNum(85, (i* MEHE)+Show_d + 6, 0, 1, OLED_6X8);
								}
							}
							
							if(Cursor==0)		OLED_ShowString(Cursor_len_d0, Cursor_i_d0+2, "<-", OLED_6X8);	//尾巴光标
							else if(Cursor==1)	OLED_DrawRectangle(0, Cursor_i_d0, Cursor_len_d0 + 8, 18, OLED_UNFILLED);
							else if(Cursor==2)	OLED_ReverseArea2 (0, Cursor_i_d0, Cursor_len_d0, 16);			//对指定位置取反
							
							if( NP_target - NP > 25 ) OLED_ClearArea(4, 2, NP + 5, NP / 2) ;
							else if( NP_target - NP > 5 ) OLED_ClearArea(4, 2, NP + 5, NP / 2);
							else if( NP_target - NP >= 0 ) OLED_ClearArea(4, 2, NP + 5, NP / 2);
							
							if( NP_target - NP > 25 ) NP -= 5 ;
							else if( NP_target - NP > 5 ) NP -= 2.5 ;
							else if( NP_target - NP >= 0 ) NP -= 1 ;
							
							OLED_DrawRectangle(4, 2, NP, NP / 2, OLED_UNFILLED);
							OLED_Update();
						}
						break;
					}
				}
			}//执行器	结束
		}
		
		if(menu_Back_event())	//退出
		{
			return;
		}
	}
}

/************************************************************************************************************************************************/

void run_menu1(struct option_class1* option)
{
	int8_t  Catch_i = 1;		    //选中下标
	int8_t  Show_i = 0; 		 	//显示起始下标
	int8_t  Max = 0;				//选项数量
	int8_t  Roll_Event = 0;	    	//按键事件
	
	while(option[++Max].Name[0] != '.');				//获取条目数量,如果文件名开头为'.'则为结尾;（循环获取'.'遇到'.'时停止循环）
	Max--;												//不打印'.'（减去'.'）
	
	for(int8_t i = 0; i <= Max; i++)					//计算选项名宽度;
	{
		option[i].NameLen = Get_NameLen(option[i].Name);
	}
	
	int8_t Show_d = 0, Show_i_temp = Max , yi = 40;		//显示动画相关
	float yp = 0, ypt = 0 ;
	
	while(1)
	{
		OLED_Clear();
		
		Roll_Event = menu_Roll_event();					//获取按键事件
		if(Roll_Event)									//如果有按键事件
		{
			Catch_i += Roll_Event;
			if(Catch_i < 0) 	{Catch_i = 0;}			//限制选中下标（		）
			if(Catch_i > Max) 	{Catch_i = Max;}
		}
/*****************************显示相关*****************************/
		Show_i = Catch_i ;				//计算显示起始下标
		
		if(Show_i - Show_i_temp)		//如果下标有偏移
		{
			Show_d = (Show_i - Show_i_temp) * 40;
			Show_i_temp = Show_i;
			yp = option[Catch_i].NameLen * WORD_H ;
		}
		if(Show_d) {Show_d /= 1.15;}	//滚动变化速度
		if(yi) {yi /= 1.3;}				//滚动变化速度

		if(ypt < yp)
		{
			if( yp - ypt > 10 ) ypt += 5 ;
			else if( yp - ypt > 5 ) ypt += 2.5 ;
			else if( yp - ypt > 0 ) ypt += 0.5 ;
		}
		if(ypt > yp)
		{
			if( ypt - yp > 10 ) ypt -= 5 ;
			else if( ypt - yp > 5 ) ypt -= 2.5 ;
			else if( ypt - yp > 0 ) ypt -= 0.5 ;
		}
		
		for(int8_t i = -2; i < 3; i++)	  //遍历显示选项名和对应参数
		{
			int8_t temp = i * 40 + Show_d + 48;
			
			if(Show_i + i < 0)	 {continue;}
			if(Show_i + i > Max) {break;}
			
			OLED_ShowImage(temp , 8 - yi , 32 , 32 , option[Show_i + i].Image);
			uint8_t tp = abs(Show_d/3);
			OLED_ShowString( 64 - yp / 2, 50 + tp, option[Catch_i].Name, OLED_6X8);
			OLED_DrawRectangle( 64 - (ypt / 2) - 2 ,63, ypt + 4 ,1 ,OLED_UNFILLED);
		}
		
/*****************************刷新屏幕*****************************/
		OLED_Update();
/******************************************************************/
		if(menu_Enter_event())
		{
			/*如果功能不为空则执行功能,否则返回*/
			if(option[Catch_i].func)
			{
				option[Catch_i].func();
				yi = 40;
				Show_d = -40;
			}else	return;
		}
		if(menu_Back_event())	return;
	}
}

/************************************************************************************************************************************************/

//******************//
//计算选项名宽度//
uint8_t Get_NameLen(char* String)
{
	uint8_t i = 0, len = 0;
	while(String[i] != '\0')			        //遍历字符串的每个字符
	{
		if(String[i] > '~'){len += 2; i += 2;}	//如果不属于英文字符，因为GB2312编码格式长度加2（i+=2），UTF-8编码格式则加3
		else{len += 1; i += 1;}				 	//属于英文字符长度加1
	}
	return len;
}

//******************//
//*计算取反光标的宽度*//
void OLED_ReverseArea2(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height)
{
	OLED_ReverseArea( X+1,	Y, 			Width-2,	1);
	OLED_ReverseArea( X, 	Y+1,		Width, 		Height-2);
	OLED_ReverseArea( X+1,	Y+Height-1, Width-1, 	1);
}

//******************//
//*定时获取和发送数据*//
uint8_t time = 0;
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		DHT11_Read_Data(&temp,&hum);
		light = Get_light();
		
		if(time == 5){
			Serial_SendString("temp:");
			Serial_SendNumber(temp, 2);
			Serial_SendString(",");
			Serial_SendString("hum:");
			Serial_SendNumber(hum, 2);
		}
		
		if(time >= 10){
			Serial_SendString("light:");
			Serial_SendNumber(light, 2);
			Serial_SendString(",");
			Serial_SendString("dja:");
			Serial_SendNumber(dianji1, 3);
			
			time = 0;
		}
		
		time++;	
		
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
