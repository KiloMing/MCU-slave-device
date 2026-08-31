/**
 ******************************************************************************
 * @file    Mulun.c
 * @brief   Original Mecanum-wheel heading PID calculation
 * @pin_resources No direct pins; Motor.c maps the output to four PWM channels.
 * @peripherals No direct peripheral.
 * @function Normalizes yaw, separates and limits integration, and limits output.
 * @purpose Produces the rotational correction used by mecanum_move().
 * @migration Equations, order, thresholds and constants match src/Hardwaer/mulun.c.
 ******************************************************************************
 */

#include "Mulun.h"

#include <stddef.h>

#define PID_MULUN_DEFAULT_P       12.0f
#define PID_MULUN_DEFAULT_I       2.0f
#define PID_MULUN_DEFAULT_D       2.0f
#define PID_MULUN_DEFAULT_I_MAX   35.0f
#define PID_MULUN_DEFAULT_OUT_MAX 350.0f
#define PID_MULUN_INTEG_SEP_THRES 30.0f

static float normalize_angle(float angle)
{
    while (angle > 180.0f)
    {
        angle -= 360.0f;
    }
    while (angle < -180.0f)
    {
        angle += 360.0f;
    }
    return angle;
}

void PID_Mulun_Init(PID_Mulun_HandleTypeDef *hpid)
{
    if (hpid == NULL)
    {
        return;
    }

    hpid->p = PID_MULUN_DEFAULT_P;
    hpid->i = PID_MULUN_DEFAULT_I;
    hpid->d = PID_MULUN_DEFAULT_D;
    hpid->integral_max = PID_MULUN_DEFAULT_I_MAX;
    hpid->output_max = PID_MULUN_DEFAULT_OUT_MAX;
    hpid->integral = 0.0f;
    hpid->last_error = 0.0f;
}

float PID_Mulun_Calc(PID_Mulun_HandleTypeDef *hpid,
                     float target_yaw,
                     float current_yaw)
{
    float current;
    float error;
    float derivative;
    float pwm;

    if (hpid == NULL)
    {
        return 0.0f;
    }

    current = normalize_angle(current_yaw);
    error = normalize_angle(target_yaw - current);

    if (hpid->i != 0.0f)
    {
        if ((error > -PID_MULUN_INTEG_SEP_THRES) &&
            (error < PID_MULUN_INTEG_SEP_THRES))
        {
            hpid->integral += error;
            if (hpid->integral > hpid->integral_max)
            {
                hpid->integral = hpid->integral_max;
            }
            else if (hpid->integral < -hpid->integral_max)
            {
                hpid->integral = -hpid->integral_max;
            }
        }
    }

    derivative = error - hpid->last_error;
    hpid->last_error = error;
    pwm = hpid->p * error + hpid->d * derivative + hpid->i * hpid->integral;

    if (pwm > hpid->output_max)
    {
        pwm = hpid->output_max;
    }
    else if (pwm < -hpid->output_max)
    {
        pwm = -hpid->output_max;
    }

    return pwm;
}
