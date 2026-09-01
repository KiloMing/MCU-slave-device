/**
 ******************************************************************************
 * @file    UART.h
 * @brief   USART2 TTL communication interface
 *
 * @pin_resources
 *   - PA2 : USART2_TX, TTL output to USB-TTL RXD.
 *   - PA3 : USART2_RX, TTL input from USB-TTL TXD.
 *   - PA6 : onboard RS485-2 DE, output low to disable its transmitter.
 *   - PA7 : onboard RS485-2 /RE, output high to disable its receiver output.
 *
 * @peripherals
 *   - USART2, GPIOA and APB1/APB2 clocks.
 *
 * @function
 *   - Initializes 8N1 USART2, dispatches received bytes, and sends diagnostics.
 *
 * @purpose
 *   - Carries the original upper-computer packet on the verified TTL pins.
 *
 * @migration
 *   - Source: E:\project_M\test_p\Hardware\UART.h from Git commit 53ca0a6.
 *   - Unchanged: public API, baud parameter, byte order and blocking behavior.
 *   - Adapted: STM32 standard-peripheral calls replaced by STM32F1 HAL calls.
 ******************************************************************************
 */

#ifndef UART_H
#define UART_H

#include "stm32f1xx_hal.h"

extern UART_HandleTypeDef huart2;

typedef void (*UART_RxByteHandler_t)(uint8_t byte);

void UART_Init(uint32_t BaudRate);
void UART_SendByte(uint8_t Byte);
void UART_SendString(const char *String);
uint8_t UART_ReceiveByte(void);
void UART_StartReceiveIT(UART_RxByteHandler_t handler);

#endif
