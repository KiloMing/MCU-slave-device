#ifndef __motor_H
#define __motor_H

#include "main.h"
#include "hardware.h"
void motor_back_pin(void);
void motor_forward_pin(void);
void motor_lf_forward_pin(void);
void motor_lf_back_pin(void);
void motor_rf_forward_pin(void);
void motor_rf_back_pin(void);
void motor_all_set(int32_t speed);
void motor_lb_forward_pin(void);
void motor_lb_back_pin(void);
void motor_rb_forward_pin(void);
void motor_rb_back_pin(void);
void motor_sp(int32_t speed);
void motor_com(int32_t speed,int8_t com);
void motor_strafe(int32_t speed);
void motor_PWM_Init(void);

void mecanum_move(int32_t vx, int32_t vy, float omega);

void mecanum_with_heading_control(uint16_t vx, uint16_t vy, float target_yaw, float current_yaw);

void Motor_LF_SetSpeed(int32_t speed);
void Motor_LB_SetSpeed(int32_t speed);
void Motor_RF_SetSpeed(int32_t speed);
void Motor_RB_SetSpeed(int32_t speed);

void Motor_Init(void);

#endif
