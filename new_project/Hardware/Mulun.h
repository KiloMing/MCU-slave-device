/**
 ******************************************************************************
 * @file    Mulun.h
 * @brief   Original Mecanum-wheel heading PID interface
 * @pin_resources No direct pins; output is consumed by the four motor channels.
 * @peripherals No direct peripheral; uses floating-point control arithmetic.
 * @function Stores and calculates the original heading PID state.
 * @purpose Keeps the chassis heading at the upper-computer target yaw.
 * @migration Structure, field types, function names and values match src/Hardwaer/mulun.h.
 ******************************************************************************
 */

#ifndef MULUN_H
#define MULUN_H

typedef struct
{
    float integral;
    float last_error;
    float p;
    float i;
    float d;
    float integral_max;
    float output_max;
} PID_Mulun_HandleTypeDef;

void PID_Mulun_Init(PID_Mulun_HandleTypeDef *hpid);
float PID_Mulun_Calc(PID_Mulun_HandleTypeDef *hpid,
                     float target_yaw,
                     float current_yaw);

#endif
