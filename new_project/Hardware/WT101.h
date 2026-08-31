    /**
 ******************************************************************************
 * @file    WT101.h
 * @brief   HWT101 raw UART-frame interface
 * @pin_resources PB6=USART1_TX, PB7=USART1_RX; @peripherals remapped USART1
 * @function Initializes UART, receives an 11-byte frame and converts 0x53 yaw.
 * @purpose Supplies signed current yaw to the original chassis heading PID.
 * @migration UART replaces the old I2C transport; yaw scale remains raw/32768*180.
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
uint8_t WT101_UART_ParseYaw(const uint8_t frame[WT101_UART_FRAME_SIZE],
                           float *yaw_angle);
HAL_StatusTypeDef WT101_UART_ReadYaw(float *yaw_angle, uint32_t timeout_ms);

#endif
