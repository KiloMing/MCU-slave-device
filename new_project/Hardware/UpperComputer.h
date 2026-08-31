/**
 ******************************************************************************
 * @file    UpperComputer.h
 * @brief   Original fixed-length upper-computer UART protocol
 * @pin_resources PA2=USART2_TX and PA3=USART2_RX through UART.c.
 * @peripherals USART2 receive interrupt.
 * @function Receives, validates and exposes the original ten-byte packet.
 * @purpose Supplies vx, vy and target yaw without altering any packet field.
 * @migration Layout, constants, globals and receive flow match usart_parse.h.
 ******************************************************************************
 */

#ifndef UPPER_COMPUTER_H
#define UPPER_COMPUTER_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef struct
{
    uint8_t header;
    uint8_t forward_speed;
    uint8_t horizontal_speed;
    uint8_t target_angle;
    uint8_t rudder_angle;
    uint8_t lift_rod;
    uint8_t horizontal_rod;
    uint8_t switch_one;
    uint8_t switch_two;
    uint8_t footer;
} UART_Packet_t;

#define UART_PACKET_LENGTH  10U
#define UART_PACKET_HEADER  0xB3U
#define UART_PACKET_FOOTER  0xB4U
#define UART_RX_BUFFER_SIZE 10U

extern uint8_t rx_data;
extern uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
extern uint8_t rx_cnt;
extern volatile uint8_t rx_complete_flag;
extern volatile uint8_t parsing_in_progress;
extern uint16_t motor_vx;
extern uint16_t motor_vy;
extern float target_yaw;

void UART_Enable_Receive(void);
void UART_Disable_Receive(void);
void UART_Parse_Data(void);
void UART_Launch(void);
const UART_Packet_t *UART_GetLatestPacket(void);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#endif
