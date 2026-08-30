    /**
 ******************************************************************************
 * @file    WT101.h
 * @brief   HWT101 raw UART-frame interface
 * @pin_resources PB6=USART1_TX, PB7=USART1_RX; @peripherals remapped USART1
 * @function Initializes HWT101 UART and receives one 11-byte raw frame.
 * @purpose Validates the sensor's original serial output before angle parsing.
 * @migration Source: HWT101 UART protocol; USART1 remapped to avoid onboard MAX232.
 ******************************************************************************
 */
#ifndef WT101_H
#define WT101_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define WT101_UART_FRAME_SIZE 11U
#define WT101_UART_BAUD_RATE  115200U

extern UART_HandleTypeDef huart1;

void WT101_UART_Init(uint32_t baud_rate);
HAL_StatusTypeDef WT101_UART_ReadFrame(uint8_t frame[WT101_UART_FRAME_SIZE],
                                      uint32_t timeout_ms);
uint8_t WT101_UART_ChecksumOK(const uint8_t frame[WT101_UART_FRAME_SIZE]);

#endif
