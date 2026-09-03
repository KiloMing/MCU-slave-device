/**
 ******************************************************************************
 * @file    stm32f1xx_it.c
 * @brief   Minimal HAL interrupt handlers
 *
 * @pin_resources
 *   - PB0/PB1: Motor C encoder EXTI0/EXTI1.
 *   - PA5/PA12: Motor D encoder EXTI9_5/EXTI15_10.
 *
 * @peripherals
 *   - Cortex-M3 core exceptions, SysTick, USART2 and GPIO EXTI.
 *
 * @function
 *   - Handles core exceptions and advances the HAL tick from SysTick.
 *
 * @purpose
 *   - Supports the clean HAL baseline without application-module dependencies.
 *
 * @migration
 *   - Replaces the legacy standard-library interrupt file.
 *   - Removed the obsolete ultrasound dependency and extra application tick.
 ******************************************************************************
 */

#include "stm32f1xx_it.h"
#include "UART.h"

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    while (1)
    {
    }
}

void MemManage_Handler(void)
{
    while (1)
    {
    }
}

void BusFault_Handler(void)
{
    while (1)
    {
    }
}

void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
}

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
}
