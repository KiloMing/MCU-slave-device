#ifndef PID_H
#define PID_H

#include "main.h"

typedef struct {
    int KP;
    int KI;
    int KD;
    int error;
    int last_error;
    int integral;
    int pid_output;
    int BASE_SPEED;
    int MAX_ADJUST;
} PID_HandleTypeDef;

typedef struct {
    int32_t KP;
    int32_t KI;
    int32_t KD;
    int32_t target_speed;
    int32_t current_speed;
    int32_t error;
    int32_t last_error;
    int32_t prev_error;
    int32_t output;
    int32_t max_output;
} SpeedPID_HandleTypeDef;

#define DEFAULT_KP 1.4
#define DEFAULT_KI 0.05
#define DEFAULT_KD 0.7
#define DEFAULT_BASE_SPEED 250
#define DEFAULT_MAX_ADJUST 200

#define DEFAULT_SPEED_KP 2
#define DEFAULT_SPEED_KI 1
#define DEFAULT_SPEED_KD 0

extern PID_HandleTypeDef pid;
extern SpeedPID_HandleTypeDef left_speed_pid;
extern SpeedPID_HandleTypeDef right_speed_pid;

void PID_Init(PID_HandleTypeDef *pid);
void PID_Compute(PID_HandleTypeDef *pid, int error);
void PID_GetParams(int *kp, int *ki, int *kd, int *base_speed);
void PID_SetParams(int kp, int ki, int kd);
void PID_SetBaseSpeed(int speed);
void PID_Reset(void);

void SpeedPID_Init(SpeedPID_HandleTypeDef *pid, int32_t kp, int32_t ki, int32_t kd, int32_t max_output);
void SpeedPID_SetParams(SpeedPID_HandleTypeDef *pid, int32_t kp, int32_t ki, int32_t kd);
int32_t SpeedPID_Compute(SpeedPID_HandleTypeDef *pid, int32_t target_speed, int32_t current_speed);
void SpeedPID_Reset(SpeedPID_HandleTypeDef *pid);

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif
