/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : pid.c
  * @brief          : PID control implementation
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "pid.h"
#include "encoder.h"
#include "motor.h"
#include "tim.h"
#include "line_trace.h"
extern uint8_t stop_request ;
/* Private variables ---------------------------------------------------------*/
PID_HandleTypeDef pid;
SpeedPID_HandleTypeDef left_speed_pid;
SpeedPID_HandleTypeDef right_speed_pid;
extern uint8_t motor_enable;

/* Private function prototypes -----------------------------------------------*/
static void Control_Motors(void);
static int32_t ClampValue(int32_t value, int32_t min_value, int32_t max_value);
static int32_t GetMotorPwmMax(void);

/* Private functions ---------------------------------------------------------*/
static int32_t ClampValue(int32_t value, int32_t min_value, int32_t max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static int32_t GetMotorPwmMax(void)
{
    return (int32_t)__HAL_TIM_GET_AUTORELOAD(&htim2);
}

/* Exported functions --------------------------------------------------------*/
void PID_Init(PID_HandleTypeDef *pid_handle)
{
    pid_handle->KP = DEFAULT_KP;
    pid_handle->KI = DEFAULT_KI;
    pid_handle->KD = DEFAULT_KD;
    pid_handle->error = 0;
    pid_handle->last_error = 0;
    pid_handle->integral = 0;
    pid_handle->pid_output = 0;
    pid_handle->BASE_SPEED = DEFAULT_BASE_SPEED;
    pid_handle->MAX_ADJUST = DEFAULT_MAX_ADJUST;

    SpeedPID_Init(&left_speed_pid, DEFAULT_SPEED_KP, DEFAULT_SPEED_KI, DEFAULT_SPEED_KD, GetMotorPwmMax());
    SpeedPID_Init(&right_speed_pid, DEFAULT_SPEED_KP, DEFAULT_SPEED_KI, DEFAULT_SPEED_KD, GetMotorPwmMax());
}

void PID_Compute(PID_HandleTypeDef *pid_handle, int error)
{
    int derivative;

    if (pid_handle->KI != 0) {
        int integral_limit;

        pid_handle->integral += error;
        integral_limit = pid_handle->MAX_ADJUST / pid_handle->KI;
        if (integral_limit < 50) {
            integral_limit = 50;
        }

        pid_handle->integral = ClampValue(pid_handle->integral, -integral_limit, integral_limit);
    } else {
        pid_handle->integral = 0;
    }

    derivative = error - pid_handle->last_error;

    pid_handle->pid_output = pid_handle->KP * error
                           + pid_handle->KI * pid_handle->integral
                           + pid_handle->KD * derivative;
    pid_handle->pid_output = ClampValue(pid_handle->pid_output,
                                        -pid_handle->MAX_ADJUST,
                                        pid_handle->MAX_ADJUST);

    pid_handle->last_error = error;
}

void SpeedPID_Init(SpeedPID_HandleTypeDef *pid_handle,
                   int32_t kp,
                   int32_t ki,
                   int32_t kd,
                   int32_t max_output)
{
    pid_handle->KP = kp;
    pid_handle->KI = ki;
    pid_handle->KD = kd;
    pid_handle->max_output = max_output;
    SpeedPID_Reset(pid_handle);
}

void SpeedPID_SetParams(SpeedPID_HandleTypeDef *pid_handle, int32_t kp, int32_t ki, int32_t kd)
{
    pid_handle->KP = kp;
    pid_handle->KI = ki;
    pid_handle->KD = kd;
}

void SpeedPID_Reset(SpeedPID_HandleTypeDef *pid_handle)
{
    pid_handle->target_speed = 0;
    pid_handle->current_speed = 0;
    pid_handle->error = 0;
    pid_handle->last_error = 0;
    pid_handle->prev_error = 0;
    pid_handle->output = 0;
}

int32_t SpeedPID_Compute(SpeedPID_HandleTypeDef *pid_handle, int32_t target_speed, int32_t current_speed)
{
    int32_t delta_output;

    pid_handle->max_output = GetMotorPwmMax();

    if (target_speed <= 0) {
        SpeedPID_Reset(pid_handle);
        return 0;
    }

    pid_handle->target_speed = target_speed;
    pid_handle->current_speed = current_speed;
    pid_handle->error = target_speed - current_speed;

    delta_output = pid_handle->KP * (pid_handle->error - pid_handle->last_error)
                 + pid_handle->KI * pid_handle->error
                 + pid_handle->KD * (pid_handle->error - 2 * pid_handle->last_error + pid_handle->prev_error);

    pid_handle->output += delta_output;
    pid_handle->output = ClampValue(pid_handle->output, 0, pid_handle->max_output);

    pid_handle->prev_error = pid_handle->last_error;
    pid_handle->last_error = pid_handle->error;

    return pid_handle->output;
}

static void Control_Motors(void)
{
    int32_t left_target_speed;
    int32_t right_target_speed;
    int32_t left_pwm;
    int32_t right_pwm;

    left_target_speed = pid.BASE_SPEED + pid.pid_output;
    right_target_speed = pid.BASE_SPEED - pid.pid_output;

    if (left_target_speed < 0) {
        left_target_speed = 0;
    }
    if (right_target_speed < 0) {
        right_target_speed = 0;
    }

    if (motor_enable == 1) {
		
         right_pwm= SpeedPID_Compute(&left_speed_pid, left_target_speed, encoder_left_speed);
      left_pwm   = SpeedPID_Compute(&right_speed_pid, right_target_speed, encoder_right_speed);

        Motor_L_SetSpeed(left_pwm);
        Motor_R_SetSpeed(right_pwm);

    } else {
        SpeedPID_Reset(&left_speed_pid);
        SpeedPID_Reset(&right_speed_pid);
        Motor_L_SetSpeed(0);
        Motor_R_SetSpeed(0);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1) {
        encoder_calculate_speed();
        PID_Compute(&pid, pid.error);
        Control_Motors();

    }
}

void PID_GetParams(int *kp, int *ki, int *kd, int *base_speed)
{
    *kp = pid.KP;
    *ki = pid.KI;
    *kd = pid.KD;
    *base_speed = pid.BASE_SPEED;
}

void PID_SetParams(int kp, int ki, int kd)
{
    pid.KP = kp;
    pid.KI = ki;
    pid.KD = kd;
}

void PID_SetBaseSpeed(int speed)
{
    pid.BASE_SPEED = speed;
}

void PID_Reset(void)
{
    PID_Init(&pid);
}
