/**
  * @file    mulun.h
  * @brief   麦克纳姆轮角度PID控制器头文件
  * @author  Auto Generated
  * @date    2024
  * @version V1.0
  */

#ifndef __mulun_H
#define __mulun_H

#include "motor.h"
#include "main.h"      
#include "tim.h"
#include <stdint.h>
#include <stdbool.h>

/**
  * @brief  PID控制器句柄结构体定义
  * @note   用于存储PID控制器的所有状态和参数
  */
typedef struct {
    float integral;        /* 积分项累计值 */
    float last_error;      /* 上一次误差值（用于计算微分） */
    float p;               /* 比例系数 */
    float i;               /* 积分系数 */
    float d;               /* 微分系数 */
    float integral_max;    /* 积分上限（防止积分饱和） */
    float output_max;      /* 输出上限（防止输出超限） */
} PID_Mulun_HandleTypeDef;

/**
  * @brief  初始化PID控制器
  * @param  hpid: PID控制器句柄指针
  * @retval None
  */
void PID_Mulun_Init(PID_Mulun_HandleTypeDef *hpid);

/**
  * @brief  计算PID输出
  * @param  hpid: PID控制器句柄指针
  * @param  target_yaw: 目标角度（单位：度）
  * @param  current_yaw: 当前角度（单位：度）
  * @retval PID输出值（用于角度校正的PWM值）
  */
float PID_Mulun_Calc(PID_Mulun_HandleTypeDef *hpid, float target_yaw, float current_yaw);

#endif /* __mulun_H */
