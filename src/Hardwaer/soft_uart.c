#include "soft_uart.h"
#include "main.h"
#include "tim.h"
#include "stm32f1xx_hal.h"

#define SOFT_UART_BIT_US 87U
#define SOFT_UART_CYCLES_PER_US (SystemCoreClock / 1000000U)

static void SoftUart_DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t target = us * SOFT_UART_CYCLES_PER_US;

    while ((uint32_t)(DWT->CYCCNT - start) < target) {
    }
}

void SoftUart_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0U;

    GPIO_InitStruct.Pin = SOFT_UART_TX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SOFT_UART_TX_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SOFT_UART_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SOFT_UART_RX_GPIO_Port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(SOFT_UART_TX_GPIO_Port, SOFT_UART_TX_Pin, GPIO_PIN_SET);
}

void SoftUart_SendByte(uint8_t byte)
{
    uint8_t i;

    HAL_GPIO_WritePin(SOFT_UART_TX_GPIO_Port, SOFT_UART_TX_Pin, GPIO_PIN_RESET);
    SoftUart_DelayUs(SOFT_UART_BIT_US);

    for (i = 0; i < 8U; i++) {
        HAL_GPIO_WritePin(SOFT_UART_TX_GPIO_Port,
                          SOFT_UART_TX_Pin,
                          (byte & 0x01U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        SoftUart_DelayUs(SOFT_UART_BIT_US);
        byte >>= 1;
    }

    HAL_GPIO_WritePin(SOFT_UART_TX_GPIO_Port, SOFT_UART_TX_Pin, GPIO_PIN_SET);
    SoftUart_DelayUs(SOFT_UART_BIT_US);
}

void SoftUart_SendBuffer(const uint8_t *data, uint16_t length)
{
    uint16_t i;
    for (i = 0; i < length; i++) {
        SoftUart_SendByte(data[i]);
    }
}

void SoftUart_SendString(const char *s)
{
    while (*s != '\0') {
        SoftUart_SendByte((uint8_t)*s++);
    }
}
