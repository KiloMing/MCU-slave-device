#ifndef __tower_H
#define __tower_H
#include "main.h"
#include "Emm_V5.h"
#include "hardware.h"



// 丝杠&齿轮步进电机硬件参数：
#define STEP_MOTOR_PULSE_PER_CIRCLE  3200    // 电机一圈脉冲数
#define LEAD_SCREW_PITCH             4.0f     // 丝杠螺距（mm/圈）
#define PULSE_PER_MM                 (STEP_MOTOR_PULSE_PER_CIRCLE / LEAD_SCREW_PITCH)  // 每毫米脉冲数800个

#define GEAR_CIRCUMFERENCE 125.66f // 齿轮周长(mm)，直径40mm，π≈3.14159


//addr 1 丝杠
void Rail_StepMotor_ControlByMM( uint8_t move_mm,uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc, bool raF, bool snF) ;

//addr 2 齿轮
void Gear_StepMotor_ControlByMM( uint8_t move_mm,uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc, bool raF, bool snF) ;


//	Gear_StepMotor_ControlByMM(50,2,0,100,5,1,0);
//	HAL_Delay(1);
//	Rail_StepMotor_ControlByMM(50,1,1,100,5,1,0);

















#endif
