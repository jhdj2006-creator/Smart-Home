#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Motor.h"
#include "Key.h"
#include "Serial.h"
#include "Serial2.h"
#include "string.h"

uint8_t KeyNum;
int8_t Speed;
uint8_t RxData2;

int main(void)
{
	OLED_Init();
	MotorAll_Init();
	Key_Init();
	Serial_Init();
	Serial2_Init();
	
	OLED_ShowString(1, 1, "Speed:");
	OLED_ShowString(3, 1, "RxPacket");

	while (1)
	{
//		if (Serial2_RxFlag == 1)
//		{
//			OLED_ShowString(4, 1, "                ");
//			OLED_ShowString(4, 1, Serial2_RxPacket);
//			
//			if (strcmp(Serial2_RxPacket, "DJA_0") == 0)
//			{
//				MotorR_SetSpeed(0);
//				Serial_SendString("@DJA_0\r\n");
//			}
//			else if (strcmp(Serial2_RxPacket, "DJA_20") == 0)
//			{
//				MotorR_SetSpeed(20);
//				Serial_SendString("@DJA_20\r\n");
//			}
//			else if (strcmp(Serial2_RxPacket, "DJA_40") == 0)
//			{
//				MotorR_SetSpeed(40);
//				Serial_SendString("@DJA_40\r\n");
//			}
//			else if (strcmp(Serial2_RxPacket, "DJA_60") == 0)
//			{
//				MotorR_SetSpeed(60);
//				Serial_SendString("@DJA_60\r\n");
//			}
//			else if (strcmp(Serial2_RxPacket, "DJA_80") == 0)
//			{
//				MotorR_SetSpeed(80);
//				Serial_SendString("@DJA_80\r\n");
//			}
//			else if (strcmp(Serial2_RxPacket, "DJA_100") == 0)
//			{
//				MotorR_SetSpeed(100);
//				Serial_SendString("@DJA_100\r\n");
//			}
//			
//			Serial2_RxFlag = 0;
//		}
		
		if (Serial_RxFlag == 1)
		{
			OLED_ShowString(4, 1, "                ");
			OLED_ShowString(4, 1, Serial_RxPacket);
			
			if (strcmp(Serial_RxPacket, "DJA_0") == 0)
			{
				MotorR_SetSpeed(0);
				OLED_ShowSignedNum(1, 7, 0, 3);
			}
			else if (strcmp(Serial_RxPacket, "DJA_20") == 0)
			{
				MotorR_SetSpeed(20);
				OLED_ShowSignedNum(1, 7, 20, 3);
				OLED_ShowSignedNum(1, 7, 20, 3);
			}
			else if (strcmp(Serial_RxPacket, "DJA_40") == 0)
			{
				MotorR_SetSpeed(40);
				OLED_ShowSignedNum(1, 7, 40, 3);
			}
			else if (strcmp(Serial_RxPacket, "DJA_60") == 0)
			{
				MotorR_SetSpeed(60);
				OLED_ShowSignedNum(1, 7, 60, 3);
			}
			else if (strcmp(Serial_RxPacket, "DJA_80") == 0)
			{
				MotorR_SetSpeed(80);
				OLED_ShowSignedNum(1, 7, 80, 3);
			}
			else if (strcmp(Serial_RxPacket, "DJA_100") == 0)
			{
				MotorR_SetSpeed(100);
				OLED_ShowSignedNum(1, 7, 100, 3);
			}
			
			Serial_RxFlag = 0;
		}
		
//		MotorR_SetSpeed(RxData);//×ó
//		MotorL_SetSpeed(RxData);//ÓÒ
	}
}
