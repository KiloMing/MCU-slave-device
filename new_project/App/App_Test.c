/**
 ******************************************************************************
 * @file    App_Test.c
 * @brief   Selectable subsystem tests and integrated chassis control
 *
 * @pin_resources
 *   - UART: PA2, PA3, PA6 and PA7 through UART.c.
 *   - Motor PWM/direction pins are provided by Motor.c.
 *   - HWT101 UART: PB6=USART1_TX and PB7=USART1_RX through WT101.c.
 *
 * @peripherals
 *   - Remapped USART1, USART2, TIM1, TIM2 and SysTick through HAL.
 *
 * @function
 *   - Runs isolated tests or the complete receive/heading/motor data flow.
 *
 * @purpose
 *   - Connects verified subsystems while retaining their original values.
 *
 * @migration
 *   - Sources: E:\project_M\test_p verified UART and motor test entries.
 *   - HWT101: receives original 11-byte 115200 8N1 frames without conversion.
 *   - Current mode: receives commands and signed yaw, then applies original PID.
 *   - Waits 1000 ms after initialization before receiving sensor data.
 ******************************************************************************
 */

#include "App_Test.h"

#include "Motor.h"
#include "UART.h"
#include "WT101.h"
#include "UpperComputer.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

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

void App_Test_WT101_Init(void)
{
    UART_Init(115200U);
    WT101_UART_Init(WT101_UART_BAUD_RATE);
    UART_SendString("HWT101 UART RAW TEST READY\r\n");
    HAL_Delay(1000U);
}

void App_Test_WT101_RunStep(void)
{
    char byte_text[4];
    uint8_t frame[WT101_UART_FRAME_SIZE];
    uint32_t index;
    HAL_StatusTypeDef status = WT101_UART_ReadFrame(frame, 500U);

    if (status == HAL_OK)
    {
        UART_SendString("HWT RAW:");
        for (index = 0U; index < WT101_UART_FRAME_SIZE; index++)
        {
            (void)snprintf(byte_text,
                           sizeof(byte_text),
                           " %02X",
                           (unsigned int)frame[index]);
            UART_SendString(byte_text);
        }
        UART_SendString(WT101_UART_ChecksumOK(frame) != 0U
                            ? " SUM=OK\r\n"
                            : " SUM=ERR\r\n");
    }
    else
    {
        (void)snprintf(byte_text,
                       sizeof(byte_text),
                       "%u",
                       (unsigned int)status);
        UART_SendString("HWT UART ERR=");
        UART_SendString(byte_text);
        UART_SendString("\r\n");
    }
}

void App_Test_Control_Init(void)
{
    UART_Init(115200U);
    WT101_UART_Init(WT101_UART_BAUD_RATE);
    Motor_Init();
    UART_Enable_Receive();
}

void App_Test_Control_RunStep(void)
{
    float current_yaw;

    if (rx_complete_flag != 0U)
    {
        UART_Parse_Data();
        UART_Launch();
    }

    if (WT101_UART_ReadYaw(&current_yaw, 500U) == HAL_OK)
    {
        mecanum_with_heading_control(motor_vx, motor_vy,
                                     target_yaw, current_yaw);
    }
}
