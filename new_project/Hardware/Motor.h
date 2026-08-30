/**
 ******************************************************************************
 * @file    Motor.h
 * @brief   Four-channel D24A motor driver interface
 *
 * @pin_resources
 *   - LF: PA0 TIM2_CH1 PWM; PB5/PB12 direction.
 *   - LB: PA1 TIM2_CH2 PWM; PB13/PB14 direction.
 *   - RF: PA8 TIM1_CH1 PWM; PB15/PA4 direction.
 *   - RB: PA11 TIM1_CH4 PWM; PB3/PB4 direction.
 *
 * @peripherals
 *   - TIM1, TIM2, GPIOA, GPIOB and AFIO.
 *
 * @function
 *   - Initializes four PWM outputs and controls signed wheel speeds.
 *
 * @purpose
 *   - Drives the four chassis motors through the D24A board.
 *
 * @migration
 *   - Sources: E:\project_M\src\Hardwaer\motor.c and verified test_p wiring.
 *   - Unchanged: signed speed meaning, PWM period 999 and direction behavior.
 *   - Adapted: PWM channels changed to avoid USART2 and CAN pin conflicts.
 ******************************************************************************
 */

#ifndef MOTOR_H
#define MOTOR_H

#include "stm32f1xx_hal.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;

void Motor_Init(void);
void motor_PWM_Init(void);
void motor_stop_all(void);
void motor_all_set(int32_t speed);
void Motor_LF_SetSpeed(int32_t speed);
void Motor_LB_SetSpeed(int32_t speed);
void Motor_RF_SetSpeed(int32_t speed);
void Motor_RB_SetSpeed(int32_t speed);

#endif
