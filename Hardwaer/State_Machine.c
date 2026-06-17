/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : State_Machine.c
  * @brief          : 状态机模块实现文件
  * @details        : 实现了状态机的核心逻辑，包括状态初始化、状态更新和状态切换
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

#include "State_Machine.h"
#include "usart_parse.h"

    uint16_t motor_vx = 0;
    uint16_t motor_vy = 0;
    float target_yaw = 0;


/* Private variables ---------------------------------------------------------*/
/**
  * @brief  状态机核心句柄
  * @details 存储状态机的当前状态和历史状态信息
  */
static State_Machine_Handle_t state_machine;

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  初始化状态处理函数
  * @details 处理系统初始化相关的逻辑
  * @param  None
  * @retval None
  */
static void State_Init_Handler(void);

/**
  * @brief  空闲状态处理函数
  * @details 处理系统空闲时的逻辑，检测串口接收完成标志
  * @param  None
  * @retval None
  */
static void State_Idle_Handler(void);

/**
  * @brief  任务1状态处理函数
  * @details 处理任务1的逻辑
  * @param  None
  * @retval None
  */
static void State_Task1_Handler(void);

/**
  * @brief  任务2状态处理函数
  * @details 处理任务2的逻辑
  * @param  None
  * @retval None
  */
static void State_Task2_Handler(void);

/**
  * @brief  UART数据解析状态处理函数
  * @details 处理串口数据解析的逻辑
  * @param  None
  * @retval None
  */
static void State_UART_Parse_Handler(void);

/**
  * @brief  状态切换函数
  * @details 处理状态切换的逻辑，更新当前状态和前一个状态
  * @param  new_state: 新状态
  * @retval None
  */
static void State_Machine_Transition(System_State_t new_state);

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  初始化状态机
  * @details 初始化状态机的初始状态为STATE_INIT
  * @param  None
  * @retval None
  */
void State_Machine_Init(void)
{
    /* 初始化状态机核心参数 */
    state_machine.current_state = STATE_INIT;
    state_machine.previous_state = STATE_INIT;
}

/**
  * @brief  更新状态机
  * @details 主循环中调用，根据当前状态执行相应的状态处理函数
  * @param  None
  * @retval None
  */
void State_Machine_Update(void)
{
    /* 状态分发：处理当前状态 */
    switch (state_machine.current_state) {
        case STATE_INIT:
            State_Init_Handler();
            break;
        case STATE_IDLE:
            State_Idle_Handler();
            break;
        case STATE_TASK1:
            State_Task1_Handler();
            break;
        case STATE_TASK2:
            State_Task2_Handler();
            break;
        case STATE_UART_PARSE:
            State_UART_Parse_Handler();
            break;
        default:
            /* 异常状态回退到空闲 */
            State_Machine_Transition(STATE_IDLE);
            break;
    }
}

/**
  * @brief  获取当前状态
  * @details 返回状态机的当前状态
  * @param  None
  * @retval System_State_t: 当前状态
  */
System_State_t State_Machine_Get_Current_State(void)
{
    return state_machine.current_state;
}

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  初始化状态处理函数
  * @details 处理系统初始化相关的逻辑，初始化完成后切换到空闲状态
  * @param  None
  * @retval None
  */
static void State_Init_Handler(void)
{
    /* 【框架预留】添加初始化逻辑 */
    motor_PWM_Init();
	  Motor_Init();
    CAN_Start(CAN_NUM);
		UART_Enable_Receive();
    Servo_Init();

    printf("Init Yaw: %.2f\n",  Read_Yaw());
	
    /* 初始化完成后切换到空闲任务 */
    State_Machine_Transition(STATE_IDLE);
}

/**
  * @brief  空闲状态处理函数
  * @details 处理系统空闲时的逻辑，检测串口接收完成标志
  * @param  None
  * @retval None
  * @note   当检测到rx_complete_flag标志有效时，切换到UART解析状态
  */
static void State_Idle_Handler(void)
{
    /* 【框架预留】添加空闲任务逻辑 */
//      printf("Init Yaw: %.2f\n",  Read_Yaw());
      float yaw = Read_Yaw();
			mecanum_with_heading_control(motor_vx, motor_vy,target_yaw ,yaw);

	
    /* 检测串口数据接收完成标志位 */
    if (rx_complete_flag)
    {
        /* 标志位有效时，启动数据解析任务 */
        State_Machine_Transition(STATE_UART_PARSE);
    }

    /* 【框架预留】添加状态切换触发逻辑（如按键、事件、超时） */
}

/**
  * @brief  任务1状态处理函数
  * @details 处理任务1的逻辑，任务完成后可返回空闲状态
  * @param  None
  * @retval None
  */
static void State_Task1_Handler(void)
{
    /* 【框架预留】添加任务1逻辑 */
    /* 【框架预留】添加状态切换触发逻辑（如任务完成、返回空闲） */
}

/**
  * @brief  任务2状态处理函数
  * @details 处理任务2的逻辑，任务完成后可返回空闲状态
  * @param  None
  * @retval None
  */
static void State_Task2_Handler(void)
{
    /* 【框架预留】添加任务2逻辑 */
    /* 【框架预留】添加状态切换触发逻辑（如任务完成、返回空闲） */
}

/**
  * @brief  UART数据解析状态处理函数
  * @details 处理串口数据解析的完整流程
  * @param  None
  * @retval None
  * @note   解析完成后自动返回空闲状态
  */
static void State_UART_Parse_Handler(void)
{
    /* 调用串口数据解析函数 */
    /* 解析函数内部完成： */
    /* 1) 设置解析中标志 */
    /* 2) 禁止接收新的串口数据 */
    /* 3) 解析数据包 */
    /* 4) 清除解析中标志 */
    /* 5) 重新使能接收 */
    printf("UART_Parse_Handler\n");
 
    UART_Parse_Data();

    /* 解析完成后返回空闲状态 */
    State_Machine_Transition(STATE_IDLE);
}

/**
  * @brief  状态切换函数
  * @details 处理状态切换的逻辑，更新当前状态和前一个状态
  * @param  new_state: 新状态
  * @retval None
  */
static void State_Machine_Transition(System_State_t new_state)
{
    if (new_state != state_machine.current_state) {
        state_machine.previous_state = state_machine.current_state;
        state_machine.current_state = new_state;
        /* 【框架预留】可添加状态切换回调、日志打印等 */
    }
}
