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
#include "stm32f1xx_hal.h"
#include "WT101.h"

#if APP_TEST_MODE != APP_TEST_BLUETOOTH
#error "The migration project must default to the open-loop Bluetooth chassis test"
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
volatile uint8_t rx_complete_flag;
uint16_t motor_vx = 11U;
uint16_t motor_vy = 22U;
float target_yaw = 33.0f;
static uint32_t wt101_baud;
static uint32_t receive_enable_count;
static uint32_t parse_count;
static uint32_t launch_count;
static uint32_t heading_count;
static uint16_t heading_vx;
static uint16_t heading_vy;
static float heading_target;
static float heading_current;

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

void WT101_UART_Init(uint32_t baud_rate)
{
    wt101_baud = baud_rate;
}

HAL_StatusTypeDef WT101_UART_ReadFrame(uint8_t frame[WT101_UART_FRAME_SIZE],
                                      uint32_t timeout_ms)
{
    (void)frame;
    (void)timeout_ms;
    return HAL_TIMEOUT;
}

uint8_t WT101_UART_ChecksumOK(const uint8_t frame[WT101_UART_FRAME_SIZE])
{
    (void)frame;
    return 0U;
}

HAL_StatusTypeDef WT101_UART_ReadYaw(float *yaw_angle, uint32_t timeout_ms)
{
    (void)timeout_ms;
    *yaw_angle = 12.5f;
    return HAL_OK;
}

void UART_Enable_Receive(void)
{
    receive_enable_count++;
}

void UART_Parse_Data(void)
{
    parse_count++;
    rx_complete_flag = 0U;
}

void UART_Launch(void)
{
    launch_count++;
}

void mecanum_with_heading_control(uint16_t vx, uint16_t vy,
                                  float requested_yaw, float current_yaw)
{
    heading_count++;
    heading_vx = vx;
    heading_vy = vy;
    heading_target = requested_yaw;
    heading_current = current_yaw;
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

static void test_integrated_control_preserves_original_data_flow(void)
{
    uint32_t motor_init_before = motor_init_count;

    App_Test_Control_Init();
    if ((uart_baud != 115200U) || (wt101_baud != 115200U))
    {
        fail("control UART baud changed");
    }
    if ((motor_init_count != motor_init_before + 1U) ||
        (receive_enable_count != 1U))
    {
        fail("control initialization sequence changed");
    }

    rx_complete_flag = 1U;
    App_Test_Control_RunStep();
    if ((parse_count != 1U) || (launch_count != 1U) || (heading_count != 1U))
    {
        fail("control receive/parse/PID sequence changed");
    }
    if ((heading_vx != 11U) || (heading_vy != 22U) ||
        (heading_target != 33.0f) || (heading_current != 12.5f))
    {
        fail("control values changed between receive and PID");
    }
}

int main(void)
{
    test_uart_flow_preserves_ready_text_and_blocking_echo();
    test_motor_flow_has_no_uart_and_preserves_timing();
    test_integrated_control_preserves_original_data_flow();
    puts("PASS: selectable UART and motor application flows");
    return 0;
}
