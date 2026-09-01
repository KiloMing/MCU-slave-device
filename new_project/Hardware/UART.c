/**
 ******************************************************************************
 * @file    UART.c
 * @brief   USART2 TTL blocking communication driver
 *
 * @pin_resources
 *   - PA2 : USART2_TX, alternate-function push-pull output.
 *   - PA3 : USART2_RX, floating input.
 *   - PA6 : onboard RS485-2 DE, held low.
 *   - PA7 : onboard RS485-2 /RE, held high.
 *
 * @peripherals
 *   - USART2, GPIOA and APB1/APB2 clocks.
 *
 * @function
 *   - Configures USART2 as 8 data bits, no parity, one stop bit.
 *   - Dispatches interrupt bytes to the selected packet parser.
 *
 * @purpose
 *   - Connects the STM32 board directly to the upper computer through USB-TTL.
 *
 * @migration
 *   - Source: E:\project_M\test_p\Hardware\UART.c from Git commit 53ca0a6.
 *   - Unchanged: PA2/PA3, RS485 disable levels, API and blocking behavior.
 *   - Adapted: standard-peripheral initialization replaced by HAL.
 ******************************************************************************
 */

#include "UART.h"

UART_HandleTypeDef huart2;
static uint8_t uart_rx_byte;
static UART_RxByteHandler_t uart_rx_handler;

void UART_Init(uint32_t BaudRate)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = BaudRate;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    (void)HAL_UART_Init(&huart2);

    HAL_NVIC_SetPriority(USART2_IRQn, 1U, 0U);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

void UART_SendByte(uint8_t Byte)
{
    (void)HAL_UART_Transmit(&huart2, &Byte, 1U, HAL_MAX_DELAY);
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
    uint8_t Byte = 0U;
    (void)HAL_UART_Receive(&huart2, &Byte, 1U, HAL_MAX_DELAY);
    return Byte;
}

void UART_StartReceiveIT(UART_RxByteHandler_t handler)
{
    uart_rx_handler = handler;
    (void)HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1U);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (uart_rx_handler != NULL)
        {
            uart_rx_handler(uart_rx_byte);
        }
        (void)HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1U);
    }
}
