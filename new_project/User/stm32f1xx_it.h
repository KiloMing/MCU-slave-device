/**
 ******************************************************************************
 * @file    stm32f1xx_it.h
 * @brief   HAL interrupt handler declarations
 *
 * @pin_resources
 *   - No direct GPIO resources.
 *
 * @peripherals
 *   - Cortex-M3 core exceptions, SysTick and USART2.
 *
 * @function
 *   - Declares the interrupt handlers required by the startup file.
 *
 * @purpose
 *   - Provides the minimal interrupt interface for the HAL baseline.
 *
 * @migration
 *   - Replaces the legacy standard-library interrupt template.
 *   - No application peripheral interrupt is enabled.
 ******************************************************************************
 */

#ifndef STM32F1XX_IT_H
#define STM32F1XX_IT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void USART2_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif
