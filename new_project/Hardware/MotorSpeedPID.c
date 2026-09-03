/**
 ******************************************************************************
 * @file    MotorSpeedPID.c
 * @brief   Four-channel incremental motor speed PID implementation
 * @pin_resources
 *   - LF output: PA0 PWM, PB5/PB12 direction.
 *   - LB output: PA1 PWM, PB13/PB14 direction.
 *   - RF output: PA8 PWM, PB15/PA4 direction.
 *   - RB output: PA11 PWM, PB3/PB4 direction.
 * @peripherals Uses existing TIM1/TIM2 motor PWM and HAL SysTick only.
 * @function Runs four independent signed speed loops every software-timed 10 ms.
 * @purpose Separates four-wheel PID output from future encoder acquisition.
 * @migration Uses the original incremental equation and gains; adds four wheels
 *            and symmetric output for forward/reverse Mecanum-wheel operation.
 ******************************************************************************
 */

#include "MotorSpeedPID.h"

#include "Motor.h"
#include "stm32f1xx_hal.h"

typedef struct
{
    int32_t kp;
    int32_t ki;
    int32_t kd;
    int32_t target_speed;
    int32_t current_speed;
    int32_t error;
    int32_t last_error;
    int32_t previous_error;
    int32_t output;
    int32_t gain_divisor;
    int32_t feedforward_pwm;
} MotorSpeedPID_Controller_t;

static MotorSpeedPID_Controller_t motor_pid[MOTOR_SPEED_PID_MOTOR_COUNT];
static int32_t motor_target_speed[MOTOR_SPEED_PID_MOTOR_COUNT];
static int32_t motor_measured_speed[MOTOR_SPEED_PID_MOTOR_COUNT];
static uint32_t motor_pid_last_tick;

static int32_t MotorSpeedPID_ClampMagnitude(int32_t value)
{
    if (value > MOTOR_SPEED_PID_OUTPUT_MAX)
    {
        return MOTOR_SPEED_PID_OUTPUT_MAX;
    }
    if (value < 0)
    {
        return 0;
    }
    return value;
}

static void MotorSpeedPID_ResetController(MotorSpeedPID_Controller_t *controller)
{
    controller->target_speed = 0;
    controller->current_speed = 0;
    controller->error = 0;
    controller->last_error = 0;
    controller->previous_error = 0;
    controller->output = 0;
}

static int32_t MotorSpeedPID_Compute(MotorSpeedPID_Controller_t *controller,
                                     int32_t target_speed,
                                     int32_t current_speed)
{
    int32_t delta_output;
    int32_t direction;
    int32_t target_magnitude;
    int32_t aligned_current_speed;
    uint8_t starting_or_reversing;

    if (target_speed == 0)
    {
        MotorSpeedPID_ResetController(controller);
        return 0;
    }

    starting_or_reversing =
        (uint8_t)((controller->target_speed == 0) ||
                  ((controller->target_speed > 0) && (target_speed < 0)) ||
                  ((controller->target_speed < 0) && (target_speed > 0)));
    if (starting_or_reversing != 0U)
    {
        MotorSpeedPID_ResetController(controller);
        controller->output = controller->feedforward_pwm;
    }

    direction = (target_speed < 0) ? -1 : 1;
    target_magnitude = (target_speed < 0) ? -target_speed : target_speed;
    aligned_current_speed = current_speed * direction;
    controller->target_speed = target_speed;
    controller->current_speed = current_speed;
    controller->error = target_magnitude - aligned_current_speed;
    delta_output = (controller->kp *
                    (controller->error - controller->last_error) +
                    controller->ki * controller->error +
                    controller->kd *
                    (controller->error - (2 * controller->last_error) +
                     controller->previous_error)) /
                   controller->gain_divisor;
    controller->output =
        MotorSpeedPID_ClampMagnitude(controller->output + delta_output);
    controller->previous_error = controller->last_error;
    controller->last_error = controller->error;
    return controller->output * direction;
}

void MotorSpeedPID_Init(void)
{
    uint32_t index;

    for (index = 0U; index < MOTOR_SPEED_PID_MOTOR_COUNT; index++)
    {
        motor_pid[index].kp = MOTOR_SPEED_PID_DEFAULT_KP;
        motor_pid[index].ki = MOTOR_SPEED_PID_DEFAULT_KI;
        motor_pid[index].kd = MOTOR_SPEED_PID_DEFAULT_KD;
        motor_pid[index].gain_divisor = 1;
        motor_pid[index].feedforward_pwm = 0;
        MotorSpeedPID_ResetController(&motor_pid[index]);
        motor_target_speed[index] = 0;
        motor_measured_speed[index] = 0;
    }
    motor_pid_last_tick = HAL_GetTick();
}

void MotorSpeedPID_SetParameters(uint8_t motor_index,
                                 int32_t kp,
                                 int32_t ki,
                                 int32_t kd,
                                 int32_t gain_divisor,
                                 int32_t feedforward_pwm)
{
    MotorSpeedPID_Controller_t *controller;

    if ((motor_index >= MOTOR_SPEED_PID_MOTOR_COUNT) ||
        (gain_divisor <= 0))
    {
        return;
    }

    controller = &motor_pid[motor_index];
    controller->kp = kp;
    controller->ki = ki;
    controller->kd = kd;
    controller->gain_divisor = gain_divisor;
    controller->feedforward_pwm =
        MotorSpeedPID_ClampMagnitude(feedforward_pwm);
    MotorSpeedPID_ResetController(controller);
}

void MotorSpeedPID_SetTargets(
    const int32_t targets[MOTOR_SPEED_PID_MOTOR_COUNT])
{
    uint32_t index;

    if (targets == NULL)
    {
        return;
    }
    for (index = 0U; index < MOTOR_SPEED_PID_MOTOR_COUNT; index++)
    {
        motor_target_speed[index] = targets[index];
    }
}

void MotorSpeedPID_SetMeasuredSpeeds(
    const int32_t measured_speeds[MOTOR_SPEED_PID_MOTOR_COUNT])
{
    uint32_t index;

    if (measured_speeds == NULL)
    {
        return;
    }
    for (index = 0U; index < MOTOR_SPEED_PID_MOTOR_COUNT; index++)
    {
        motor_measured_speed[index] = measured_speeds[index];
    }
}

uint8_t MotorSpeedPID_RunStep(void)
{
    int32_t output[MOTOR_SPEED_PID_MOTOR_COUNT];
    uint32_t now = HAL_GetTick();
    uint32_t index;

    if ((now - motor_pid_last_tick) < MOTOR_SPEED_PID_PERIOD_MS)
    {
        return 0U;
    }
    motor_pid_last_tick = now;

    for (index = 0U; index < MOTOR_SPEED_PID_MOTOR_COUNT; index++)
    {
        output[index] = MotorSpeedPID_Compute(&motor_pid[index],
                                              motor_target_speed[index],
                                              motor_measured_speed[index]);
    }

    Motor_LF_SetSpeed(output[0]);
    Motor_LB_SetSpeed(output[1]);
    Motor_RF_SetSpeed(output[2]);
    Motor_RB_SetSpeed(output[3]);
    return 1U;
}

void Motor_A_SetTargetSpeed(int32_t speed)
{
    motor_target_speed[0] = speed;
}

void Motor_B_SetTargetSpeed(int32_t speed)
{
    motor_target_speed[1] = speed;
}

void Motor_C_SetTargetSpeed(int32_t speed)
{
    motor_target_speed[2] = speed;
}

void Motor_D_SetTargetSpeed(int32_t speed)
{
    motor_target_speed[3] = speed;
}

int32_t MotorSpeedPID_GetTarget(uint8_t motor_index)
{
    return (motor_index < MOTOR_SPEED_PID_MOTOR_COUNT) ?
           motor_target_speed[motor_index] : 0;
}

int32_t MotorSpeedPID_GetMeasuredSpeed(uint8_t motor_index)
{
    return (motor_index < MOTOR_SPEED_PID_MOTOR_COUNT) ?
           motor_measured_speed[motor_index] : 0;
}

int32_t MotorSpeedPID_GetOutput(uint8_t motor_index)
{
    if (motor_index >= MOTOR_SPEED_PID_MOTOR_COUNT)
    {
        return 0;
    }
    return (motor_pid[motor_index].target_speed < 0) ?
           -motor_pid[motor_index].output : motor_pid[motor_index].output;
}
