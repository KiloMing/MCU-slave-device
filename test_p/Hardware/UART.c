#include "UART.h"

void UART_Init(uint32_t BaudRate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    /* Disable the onboard RS485-2 transceiver for direct TTL operation. */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOA, GPIO_Pin_6); /* DE = 0 */
    GPIO_SetBits(GPIOA, GPIO_Pin_7);   /* /RE = 1, RO is high-Z */

    /* USART2_TX: PA2. */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* USART2_RX: PA3. */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_BaudRate = BaudRate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);
}

void UART_SendByte(uint8_t Byte)
{
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
    {
    }

    USART_SendData(USART2, Byte);
}

void UART_SendString(const char *String)
{
    while (*String != '\0')
    {
        UART_SendByte((uint8_t)*String);
        String++;
    }
}

uint8_t UART_ReceiveByte(void)
{
    while (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET)
    {
    }

    return (uint8_t)USART_ReceiveData(USART2);
}
