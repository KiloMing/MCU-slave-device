/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usart_parse.c
  * @brief          : 串口解析实现文件
  * @details        : 实现串口1数据接收与验证功能，支持10字节固定长度数据包
  *                   数据包格式：包头(0xb3) + 8数据字段 + 包尾(0xb4)
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usart_parse.h"
#include "pid.h"
#include "motor.h"
#include <stdlib.h>

/* Exported variables --------------------------------------------------------*/
uint8_t rx_data;                        /*!< 串口接收单字节 */
uint8_t rx_buffer[UART_RX_BUFFER_SIZE]; /*!< 串口接收缓冲区 */
uint8_t rx_cnt = 0;                     /*!< 串口接收计数 */
uint8_t motor_enable = 0;               /*!< 电机使能标志：0-停止，1-运行 */
volatile uint8_t rx_complete_flag = 0;     /*!< 串口接收完成标志：0-未完成，1-接收完成 */
volatile uint8_t parsing_in_progress = 0;  /*!< 数据解析中标志：0-空闲，1-解析中 */

static UART_Packet_t rx_packet;  /*!< 解析后的数据包缓存 */

static UART_Packet_t last_packet; /*!< 上一个有效数据包缓存，用于数据赋值操作 */

/* Private function prototypes -----------------------------------------------*/
static uint8_t UART_Validate_Packet(const uint8_t *buffer, uint8_t length);

/**
  * @brief  使能串口接收
  * @retval None
  * @note   开启UART接收中断，允许接收新的串口数据
  */
void UART_Enable_Receive(void)
{
    rx_complete_flag = 0;
    rx_cnt = 0;
    HAL_UART_Receive_IT(&huart1, &rx_data, 1);
}

/**
  * @brief  禁止串口接收
  * @retval None
  * @note   关闭UART接收中断，禁止在解析过程中接收新数据
  */
void UART_Disable_Receive(void)
{
    parsing_in_progress = 1;
}

/**
  * @brief  验证数据包有效性
  * @param  buffer: 数据缓冲区指针
  * @param  length: 数据长度
  * @retval uint8_t: 0-无效，1-有效
  * @note   检查数据长度是否为10字节，包头是否为0xb3，包尾是否为0xb4
  */
static uint8_t UART_Validate_Packet(const uint8_t *buffer, uint8_t length)
{
    /* 验证1：检查数据长度是否为10字节 */
    if (length != UART_PACKET_LENGTH) {
        return 0;
    }

    /* 验证2：检查包头是否为0xb3 */
    if (buffer[0] != UART_PACKET_HEADER) {
        return 0;
    }

    /* 验证3：检查包尾是否为0xb4 */
    if (buffer[UART_PACKET_LENGTH - 1] != UART_PACKET_FOOTER) {
        return 0;
    }

    return 1;
}

/**
  * @brief  串口接收中断回调函数
  * @param  huart: UART句柄
  * @retval None
  * @note   当接收到单字节数据时触发，在解析中标志有效时丢弃新数据
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        /* 如果正在解析数据，则丢弃新接收的数据，不存入缓冲区 */
        if (parsing_in_progress) {
            HAL_UART_Receive_IT(&huart1, &rx_data, 1);
            return;
        }

        /* 将接收到的数据存入缓冲区 */
        rx_buffer[rx_cnt++] = rx_data;

        /* 检查是否达到10字节（用于新协议数据包） */
        if (rx_cnt >= UART_PACKET_LENGTH)
        {
            /* 验证数据包有效性 */
            if (UART_Validate_Packet(rx_buffer, rx_cnt))
            {
                /* 设置接收完成标志 */
                rx_complete_flag = 1;
            }
            else
            {
                /* 数据包无效，重置计数器 */
                rx_cnt = 0;
            }
        }
        else if (rx_cnt >= sizeof(rx_buffer))
        {
            /* 缓冲区溢出，重置计数器 */
            rx_cnt = 0;
        }

        /* 重新开启接收中断（只有未设置rx_complete_flag时才开启） */
        if (!rx_complete_flag)
        {
            HAL_UART_Receive_IT(&huart1, &rx_data, 1);
        }
    }
}

/**
  * @brief  数据解析函数
  * @retval None
  * @note   在状态机中被调用，完成数据包的解析流程
  *         1)设置解析中标志
  *         2)禁止接收新的串口数据
  *         3)解析数据包（仅完成解析流程，不进行数据赋值）
  *         4)清除解析中标志
  *         5)重置接收计数器
  *         6)清除接收完成标志
  *         7)重新使能接收
  */
void UART_Parse_Data(void)
{
    /* 检查是否有数据需要解析 */
    if (!rx_complete_flag) {
        return;
    }

    /* 步骤1：设置解析中标志 */
    parsing_in_progress = 1;

    /* 步骤2：禁止接收新的串口数据（防止中断干扰解析过程） */
    /* 注意：这里只是设置标志位，不实际禁用中断，因为中断已由回调中的检查处理 */

    /* 步骤3：解析数据包（仅完成解析流程，不进行数据赋值） */
    /* 数据包格式：0xb3 | forward_speed | horizontal_speed | target_angle | rudder_angle | lift_rod | horizontal_rod | switch_one | switch_two | 0xb4 */
    rx_packet.header = rx_buffer[0];
    rx_packet.forward_speed = rx_buffer[1];
    rx_packet.horizontal_speed = rx_buffer[2];
    rx_packet.target_angle = rx_buffer[3];
    rx_packet.rudder_angle = rx_buffer[4];
    rx_packet.lift_rod = rx_buffer[5];
    rx_packet.horizontal_rod = rx_buffer[6];
    rx_packet.switch_one = rx_buffer[7];
    rx_packet.switch_two = rx_buffer[8];
    rx_packet.footer = rx_buffer[9];

    /* switch_one controls the pump; switch_two remains reserved. */
    MilkMonitor_SetPumpRequest(rx_packet.switch_one != 0U);


    /* 步骤5：清除解析中标志 */
    parsing_in_progress = 0;

    /* 步骤6：先重置计数器，防止新数据在步骤7之前到达时被写到错误位置 */
    rx_cnt = 0;

    /* 步骤7：清除接收完成标志 */
    rx_complete_flag = 0;

    /* 步骤8：重新使能串口接收，准备接收下一个数据包 */
    HAL_UART_Receive_IT(&huart1, &rx_data, 1);
}



void UART_Launch(void){

    motor_vx = rx_packet.forward_speed;
    motor_vy = rx_packet.horizontal_speed;
    target_yaw = rx_packet.target_angle;
    Servo_SetAngle_2(rx_packet.rudder_angle);
    if (rx_packet.horizontal_rod != last_packet.horizontal_rod)
    {
        Gear_StepMotor_ControlByMM(rx_packet.horizontal_rod,2,0,10,5,1,0);
        last_packet.horizontal_rod = rx_packet.horizontal_rod; // 更新last_packet中的horizontal_rod字段  
    }
	HAL_Delay(1);
    if (rx_packet.lift_rod != last_packet.lift_rod)
    {
        Rail_StepMotor_ControlByMM(rx_packet.lift_rod,1,1,100,5,1,0);
        last_packet.lift_rod = rx_packet.lift_rod; // 更新last_packet中的lift_rod字段  
    }

//开关我先不做处理 你们想用哪个用哪个

}
