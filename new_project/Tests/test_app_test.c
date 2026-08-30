/**
 ******************************************************************************
 * @file    test_app_test.c
 * @brief   Host behavior tests for the selectable migration test entry
 *
 * @pin_resources
 *   - Uses the UART and motor resources documented by App_Test.h.
 *
 * @peripherals
 *   - USART2, TIM1, TIM2 and SysTick behavior boundaries.
 *
 * @function
 *   - Verifies the default test, UART echo flow and complete motor cycle.
 *
 * @purpose
 *   - Prevents application-level migration behavior from diverging.
 ******************************************************************************
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "App_Test.h"

#if APP_TEST_MODE != APP_TEST_MOTOR
#error "The migration project must default to the isolated four-motor test"
#endif

static uint32_t uart_baud;
static char uart_string[64];
static uint8_t uart_sent_byte;
static uint32_t uart_sent_byte_count;
static uint8_t uart_received_byte = 0xA5U;
static uint32_t motor_init_count;
static int32_t motor_speeds[4];
static size_t motor_speed_count;
static uint32_t delays[4];
static size_t delay_count;

static void fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
}

void UART_Init(uint32_t BaudRate)
{
    uart_baud = BaudRate;
}

void UART_SendByte(uint8_t Byte)
{
    uart_sent_byte = Byte;
    uart_sent_byte_count++;
}

void UART_SendString(const char *String)
{
    (void)strncpy(uart_string, String, sizeof(uart_string) - 1U);
    uart_string[sizeof(uart_string) - 1U] = '\0';
}

uint8_t UART_ReceiveByte(void)
{
    return uart_received_byte;
}

void Motor_Init(void)
{
    motor_init_count++;
}

void motor_all_set(int32_t speed)
{
    motor_speeds[motor_speed_count++] = speed;
}

void HAL_Delay(uint32_t delay_ms)
{
    delays[delay_count++] = delay_ms;
}

static void test_uart_flow_preserves_ready_text_and_blocking_echo(void)
{
    App_Test_UART_Init();
    if (uart_baud != 115200U)
    {
        fail("UART test baud changed");
    }
    if (strcmp(uart_string, "USART2 TTL TEST READY\r\n") != 0)
    {
        fail("UART ready text changed");
    }

    App_Test_UART_ProcessByte();
    if ((uart_sent_byte_count != 1U) || (uart_sent_byte != 0xA5U))
    {
        fail("UART byte was not echoed unchanged");
    }
}

static void test_motor_flow_has_no_uart_and_preserves_timing(void)
{
    uint32_t uart_calls_before = uart_sent_byte_count;

    App_Test_Motor_Init();
    App_Test_Motor_RunCycle();

    if (motor_init_count != 1U)
    {
        fail("motor initialization count changed");
    }
    if ((motor_speed_count != 2U) ||
        (motor_speeds[0] != 500) || (motor_speeds[1] != -500))
    {
        fail("motor forward/reverse sequence changed");
    }
    if ((delay_count != 2U) ||
        (delays[0] != 10000U) || (delays[1] != 10000U))
    {
        fail("motor 10-second timing changed");
    }
    if (uart_sent_byte_count != uart_calls_before)
    {
        fail("motor test produced UART byte output");
    }
}

int main(void)
{
    test_uart_flow_preserves_ready_text_and_blocking_echo();
    test_motor_flow_has_no_uart_and_preserves_timing();
    puts("PASS: selectable UART and motor application flows");
    return 0;
}
