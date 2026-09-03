/**
 ******************************************************************************
 * @file    stm32f1xx_it.h
 * @brief   HAL interrupt handler declarations
 *
 * @pin_resources
 *   - PB0/PB1 and PA5/PA12 motor encoder EXTI lines.
 *
 * @peripherals
 *   - Cortex-M3 core exceptions, SysTick, USART2 and GPIO EXTI.
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
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif
