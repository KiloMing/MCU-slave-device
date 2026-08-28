#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"

void UART_Init(uint32_t BaudRate);
void UART_SendByte(uint8_t Byte);
void UART_SendString(const char *String);
uint8_t UART_ReceiveByte(void);

#endif
