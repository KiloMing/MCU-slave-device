/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : State_Machine.h
  * @brief          : 状态机模块头文件
  * @details        : 该模块提供了一个通用的状态机框架，用于管理系统的不同状态和状态转换
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef STATE_MACHINE_FRAME_H
#define STATE_MACHINE_FRAME_H

/* Includes ------------------------------------------------------------------*/
/* 仅保留标准库依赖 */
#include <stdint.h>
#include <stdbool.h>
#include "hardware.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  系统状态枚举
  * @details 定义了系统的所有可能状态
  */
typedef enum {
    STATE_INIT = 0,    /*!< 初始化状态：系统启动时的初始状态 */
    STATE_IDLE,        /*!< 空闲状态：系统等待任务的状态 */
    STATE_TASK1,       /*!< 任务1状态：执行任务1的状态 */
    STATE_TASK2,       /*!< 任务2状态：执行任务2的状态 */
    STATE_UART_PARSE,  /*!< UART数据解析状态：解析串口接收到的数据包 */
    STATE_MAX          /*!< 状态总数（用于边界检查和扩展） */
} System_State_t;

/**
  * @brief  状态机核心句柄
  * @details 用于存储状态机的当前状态和历史状态信息
  */
typedef struct {
    System_State_t current_state;  /*!< 当前状态 */
    System_State_t previous_state; /*!< 前一个状态 */
} State_Machine_Handle_t;

extern uint16_t motor_vx;
extern uint16_t motor_vy;
extern float target_yaw;
/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  初始化状态机
  * @details 初始化状态机的初始状态为STATE_INIT
  * @param  None
  * @retval None
  */
void State_Machine_Init(void);

/**
  * @brief  更新状态机
  * @details 主循环中调用，根据当前状态执行相应的状态处理函数
  * @param  None
  * @retval None
  */
void State_Machine_Update(void);

/**
  * @brief  获取当前状态
  * @details 返回状态机的当前状态
  * @param  None
  * @retval System_State_t: 当前状态
  */
System_State_t State_Machine_Get_Current_State(void);

#endif /* STATE_MACHINE_FRAME_H */
