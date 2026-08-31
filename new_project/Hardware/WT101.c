/**
 ******************************************************************************
 * @file    WT101.c
 * @brief   HWT101 raw UART-frame driver
 * @pin_resources PB6=USART1_TX, PB7=USART1_RX; @peripherals remapped USART1
 * @function Receives 11-byte frames, verifies checksum and decodes 0x53 yaw.
 * @purpose Supplies signed yaw while retaining raw frame access for diagnosis.
 * @migration USART1 replaces I2C only; original yaw conversion scale is retained.
 ******************************************************************************
 */
#include "WT101.h"

UART_HandleTypeDef huart1;

void WT101_UART_Init(uint32_t baud_rate)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_AFIO_REMAP_USART1_ENABLE();

    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = baud_rate;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    (void)HAL_UART_Init(&huart1);
}

HAL_StatusTypeDef WT101_UART_ReadFrame(uint8_t frame[WT101_UART_FRAME_SIZE],
                                      uint32_t timeout_ms)
{
    HAL_StatusTypeDef status;
    uint8_t byte = 0U;
    uint32_t start_tick;
    uint32_t elapsed;

    if (frame == NULL)
    {
        return HAL_ERROR;
    }

    start_tick = HAL_GetTick();
    do
    {
        elapsed = HAL_GetTick() - start_tick;
        if (elapsed >= timeout_ms)
        {
            return HAL_TIMEOUT;
        }

        status = HAL_UART_Receive(&huart1,
                                  &byte,
                                  1U,
                                  timeout_ms - elapsed);
        if (status != HAL_OK)
        {
            return status;
        }
    } while (byte != 0x55U);

    frame[0] = byte;
    return HAL_UART_Receive(&huart1,
                            &frame[1],
                            WT101_UART_FRAME_SIZE - 1U,
                            20U);
}

uint8_t WT101_UART_ChecksumOK(const uint8_t frame[WT101_UART_FRAME_SIZE])
{
    uint8_t sum = 0U;
    uint32_t index;

    if (frame == NULL)
    {
        return 0U;
    }

    for (index = 0U; index < (WT101_UART_FRAME_SIZE - 1U); index++)
    {
        sum = (uint8_t)(sum + frame[index]);
    }

    return (sum == frame[WT101_UART_FRAME_SIZE - 1U]) ? 1U : 0U;
}

uint8_t WT101_UART_ParseYaw(const uint8_t frame[WT101_UART_FRAME_SIZE],
                           float *yaw_angle)
{
    int16_t yaw_raw;

    if ((frame == NULL) || (yaw_angle == NULL))
    {
        return 0U;
    }
    if ((frame[0] != 0x55U) || (frame[1] != 0x53U) ||
        (WT101_UART_ChecksumOK(frame) == 0U))
    {
        return 0U;
    }

    yaw_raw = (int16_t)(((uint16_t)frame[7] << 8U) | frame[6]);
    *yaw_angle = (float)yaw_raw / 32768.0f * 180.0f;
    return 1U;
}

HAL_StatusTypeDef WT101_UART_ReadYaw(float *yaw_angle, uint32_t timeout_ms)
{
    uint8_t frame[WT101_UART_FRAME_SIZE];
    uint32_t start_tick;
    HAL_StatusTypeDef status;

    if (yaw_angle == NULL)
    {
        return HAL_ERROR;
    }

    start_tick = HAL_GetTick();
    do
    {
        uint32_t elapsed = HAL_GetTick() - start_tick;
        if (elapsed >= timeout_ms)
        {
            return HAL_TIMEOUT;
        }
        status = WT101_UART_ReadFrame(frame, timeout_ms - elapsed);
        if (status != HAL_OK)
        {
            return status;
        }
        if (WT101_UART_ParseYaw(frame, yaw_angle) != 0U)
        {
            return HAL_OK;
        }
    } while (1);
}
