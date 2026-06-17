#include "tim.h"
#include "stm32f1xx.h"
#include "stdio.h"
#include "Servo.h"
#include "string.h"
#include "math.h"
#include "stdarg.h"


#define MIN_PULSE_WIDTH 500    // 最小脉宽 (us)
#define MAX_PULSE_WIDTH 2500   // 最大脉宽 (us)



void Servo_Init(void){

	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

}


void Servo_SetAngle_1(uint8_t Angle)
{
	
      // 限制舵机角度范围为0~270度
    if (Angle < 0)     Angle = 0;
    if (Angle > 256)   Angle = 256;

    // 角度、脉宽转换：0度对应500us，270度对应2500us，总范围270度
    // 公式：脉宽 = 最小脉宽 + (当前角度 / 总角度范围) * (最大脉宽 - 最小脉宽)
    uint32_t pulse_width = MIN_PULSE_WIDTH + ((uint32_t)Angle * (MAX_PULSE_WIDTH - MIN_PULSE_WIDTH)) / 270;

	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse_width );	
											
}




void Servo_SetAngle_2(uint8_t Angle)
{
	   // 限制舵机角度范围为0~180度
    if (Angle < 0)     Angle = 0;
    if (Angle > 180)   Angle = 180;

    // 角度、脉宽转换：0度对应500us，180度对应2500us，总范围180度
    // 公式：脉宽 = 最小脉宽 + (当前角度 / 总角度范围) * (最大脉宽 - 最小脉宽)
    uint32_t pulse_width = MIN_PULSE_WIDTH + ((uint32_t)Angle * (MAX_PULSE_WIDTH - MIN_PULSE_WIDTH)) / 180;
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse_width );	
											
}
