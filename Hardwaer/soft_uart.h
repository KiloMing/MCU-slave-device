#ifndef SOFT_UART_H
#define SOFT_UART_H

#include <stdint.h>
#include <stdbool.h>

void SoftUart_Init(void);
void SoftUart_SendByte(uint8_t byte);
void SoftUart_SendBuffer(const uint8_t *data, uint16_t length);
void SoftUart_SendString(const char *s);

#endif /* SOFT_UART_H */
