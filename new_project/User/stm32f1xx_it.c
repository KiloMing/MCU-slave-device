/**
 ******************************************************************************
 * @file    stm32f1xx_it.c
 * @brief   Minimal HAL interrupt handlers
 *
 * @pin_resources
 *   - No direct GPIO resources.
 *
 * @peripherals
 *   - Cortex-M3 core exceptions and SysTick.
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
