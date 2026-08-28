#include "stm32f10x.h"
#include "UART.h"

int main(void)
{
    uint8_t received;

    UART_Init(115200);
    UART_SendString("USART2 TTL TEST READY\r\n");

    while (1)
    {
        received = UART_ReceiveByte();
        UART_SendByte(received);
    }
}
