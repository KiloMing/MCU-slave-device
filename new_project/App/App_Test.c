/**
 ******************************************************************************
 * @file    App_Test.c
 * @brief   Selectable UART and motor migration-test behavior
 *
 * @pin_resources
 *   - UART: PA2, PA3, PA6 and PA7 through UART.c.
 *   - Motor PWM/direction pins are provided by Motor.c.
 *
 * @peripherals
 *   - USART2, TIM1, TIM2 and SysTick through the hardware modules and HAL.
 *
 * @function
 *   - Runs one blocking UART echo step or one 20-second motor test cycle.
 *
 * @purpose
 *   - Preserves the completed test_p behavior without combining both tests.
 *
 * @migration
 *   - Sources: E:\project_M\test_p verified UART and motor test entries.
 *   - Unchanged: 115200 8N1, ready text, PWM 500 and 10-second intervals.
 ******************************************************************************
 */

#include "App_Test.h"

#include "Motor.h"
#include "UART.h"
#include "stm32f1xx_hal.h"

void App_Test_UART_Init(void)
{
    UART_Init(115200U);
    UART_SendString("USART2 TTL TEST READY\r\n");
}

void App_Test_UART_ProcessByte(void)
{
    uint8_t received = UART_ReceiveByte();
    UART_SendByte(received);
}

void App_Test_Motor_Init(void)
{
    Motor_Init();
}

void App_Test_Motor_RunCycle(void)
{
    motor_all_set(500);
    HAL_Delay(10000U);
    motor_all_set(-500);
    HAL_Delay(10000U);
}
