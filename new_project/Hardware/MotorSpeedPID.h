/**
 ******************************************************************************
 * @file    MotorSpeedPID.h
 * @brief   Four-channel incremental motor speed PID interface
 * @pin_resources
 *   - LF output: PA0 PWM, PB5/PB12 direction.
 *   - LB output: PA1 PWM, PB13/PB14 direction.
 *   - RF output: PA8 PWM, PB15/PA4 direction.
 *   - RB output: PA11 PWM, PB3/PB4 direction.
 * @peripherals Uses existing TIM1/TIM2 motor PWM and HAL SysTick only.
 * @function Converts four target/measured speeds into four signed PWM outputs.
 * @purpose Provides the four PID control layer before encoder acquisition.
 * @migration Keeps original speed PID gains 2/1/0 and 10 ms update period.
 ******************************************************************************
 */

#ifndef MOTOR_SPEED_PID_H
#define MOTOR_SPEED_PID_H

#include <stdint.h>

#define MOTOR_SPEED_PID_MOTOR_COUNT 4U
#define MOTOR_SPEED_PID_DEFAULT_KP  2
#define MOTOR_SPEED_PID_DEFAULT_KI  1
#define MOTOR_SPEED_PID_DEFAULT_KD  0
#define MOTOR_SPEED_PID_OUTPUT_MAX  699
#define MOTOR_SPEED_PID_PERIOD_MS   10U

void MotorSpeedPID_Init(void);
void MotorSpeedPID_SetTargets(const int32_t targets[MOTOR_SPEED_PID_MOTOR_COUNT]);
void MotorSpeedPID_SetMeasuredSpeeds(
    const int32_t measured_speeds[MOTOR_SPEED_PID_MOTOR_COUNT]);
void MotorSpeedPID_SetParameters(uint8_t motor_index,
                                 int32_t kp,
                                 int32_t ki,
                                 int32_t kd,
                                 int32_t gain_divisor,
                                 int32_t feedforward_pwm);
uint8_t MotorSpeedPID_RunStep(void);
void Motor_A_SetTargetSpeed(int32_t speed);
void Motor_B_SetTargetSpeed(int32_t speed);
void Motor_C_SetTargetSpeed(int32_t speed);
void Motor_D_SetTargetSpeed(int32_t speed);
int32_t MotorSpeedPID_GetTarget(uint8_t motor_index);
int32_t MotorSpeedPID_GetMeasuredSpeed(uint8_t motor_index);
int32_t MotorSpeedPID_GetOutput(uint8_t motor_index);

#endif
