/**
 ******************************************************************************
 * @file    I2C.c
 * @brief   I2C1 initialization for WT101 migration test
 * @pin_resources PB6=SCL, PB7=SDA (open-drain alternate function)
 * @peripherals I2C1
 * @function Initializes a 100 kHz I2C1 bus.
 * @purpose Connects the WT101 sensor without using CAN PB8/PB9.
 * @migration Source: src/Core/Src/i2c.c; adapted to default PB6/PB7 mapping.
 ******************************************************************************
 */
#include "I2C.h"
I2C_HandleTypeDef hi2c1;
void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000U;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0U;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0U;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) { while (1) {} }
}
void HAL_I2C_MspInit(I2C_HandleTypeDef *handle)
{
    if (handle->Instance == I2C1) {
        GPIO_InitTypeDef gpio = {0};
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_I2C1_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        gpio.Mode = GPIO_MODE_AF_OD;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &gpio);
    }
}
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *handle)
{
    if (handle->Instance == I2C1) {
        __HAL_RCC_I2C1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6 | GPIO_PIN_7);
    }
}
