/**
 ******************************************************************************
 * @file    MotorEncoder.h
 * @brief   Four-channel chassis motor encoder interface
 * @pin_resources
 *   - Motor A/LF encoder: PB6=TIM4_CH1, PB7=TIM4_CH2.
 *   - Motor B/LB encoder: PA6=TIM3_CH1, PA7=TIM3_CH2.
 *   - Motor C/RF encoder: PB0=phase A, PB1=phase B, EXTI both edges.
 *   - Motor D/RB encoder: PA5=phase A, PA12=phase B, EXTI both edges.
 * @peripherals TIM3, TIM4, GPIOA, GPIOB and EXTI.
 * @function Samples four signed encoder pulse deltas and converts them to speed.
 * @purpose Supplies independent feedback to the four motor speed PID loops.
 * @migration Retains the original 10 ms, 11-line, 15-ratio and scale-2 formula.
 ******************************************************************************
 */

#ifndef MOTOR_ENCODER_H
#define MOTOR_ENCODER_H

#include <stdint.h>

#define MOTOR_ENCODER_COUNT 4U
#define MOTOR_ENCODER_SAMPLE_PERIOD_MS 10
#define MOTOR_ENCODER_RESOLUTION 11
#define MOTOR_ENCODER_GEAR_RATIO 15
#define MOTOR_ENCODER_SPEED_SCALE 2

void MotorEncoder_Init(void);
void MotorEncoder_Sample(int32_t pulses[MOTOR_ENCODER_COUNT],
                         int32_t speeds[MOTOR_ENCODER_COUNT]);
int32_t MotorEncoder_PulseToSpeed(int32_t pulse_count);
int8_t MotorEncoder_DecodeStep(uint8_t previous_state, uint8_t current_state);
void MotorEncoder_EXTI_Callback(uint16_t gpio_pin);

#endif
