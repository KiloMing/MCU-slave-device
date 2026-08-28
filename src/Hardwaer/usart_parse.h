/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usart_parse.h
  * @brief          : 串口解析头文件
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

#ifndef USART_PARSE_H
#define USART_PARSE_H

/* Includes ------------------------------------------------------------------*/
#include "hardware.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief 数据包结构体定义
  * @note  固定10字节格式：包头 + 8数据字段 + 包尾
  */
typedef struct {
    uint8_t header;            /*!< 包头，固定值0xb3 */
    uint8_t forward_speed;     /*!< 小车前进速度 */
    uint8_t horizontal_speed;  /*!< 小车水平速度 */
    uint8_t target_angle;      /*!< 小车目标角度 */
    uint8_t rudder_angle;      /*!< 水平舵机角度 */
    uint8_t lift_rod;          /*!< 升降杆 */
    uint8_t horizontal_rod;    /*!< 水平杆 */
    uint8_t switch_one;        /*!< 开关一 */
    uint8_t switch_two;        /*!< 开关二 */
    uint8_t footer;            /*!< 包尾，固定值0xb4 */
} UART_Packet_t;

/* Exported constants --------------------------------------------------------*/
#define UART_PACKET_LENGTH    10      /*!< 数据包固定长度 */
#define UART_PACKET_HEADER    0xB3    /*!< 包头固定值 */
#define UART_PACKET_FOOTER    0xB4    /*!< 包尾固定值 */
#define UART_RX_BUFFER_SIZE   10      /*!< 接收缓冲区大小 */

/* Exported variables --------------------------------------------------------*/
extern uint8_t rx_data;
extern uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
extern uint8_t rx_cnt;
extern uint8_t motor_enable;
extern volatile uint8_t rx_complete_flag;    /*!< 串口接收完成标志 */
extern volatile uint8_t parsing_in_progress; /*!< 数据解析中标志 */

/* Exported functions prototypes ---------------------------------------------*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void UART_Parse_Data(void);
void UART_Enable_Receive(void);
void UART_Disable_Receive(void);
void UART_Launch(void);

#endif /* USART_PARSE_H */
