/**
 ******************************************************************************
 * @file    test_control_algorithms.c
 * @brief   Host regression tests for the original heading PID and WT101 data
 * @pin_resources No physical pins are used by this host test.
 * @peripherals Models USART1 WT101 protocol data without target hardware.
 * @function Verifies signed yaw conversion and unchanged Mulun PID arithmetic.
 * @purpose Prevents migration from changing source data or calculations.
 ******************************************************************************
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "Mulun.h"
#include "WT101.h"

GPIO_TypeDef test_gpioa = {1U};
GPIO_TypeDef test_gpiob = {2U};
uint32_t test_usart1_instance;
uint32_t test_usart2_instance;
uint32_t test_tim1_instance;
uint32_t test_tim2_instance;

void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init)
{
    (void)port;
    (void)init;
}

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart)
{
    (void)huart;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *data,
                                  uint16_t length, uint32_t timeout)
{
    (void)huart;
    (void)data;
    (void)length;
    (void)timeout;
    return HAL_TIMEOUT;
}

uint32_t HAL_GetTick(void)
{
    return 0U;
}

static void fail_float(const char *name, float actual, float expected)
{
    if (fabsf(actual - expected) > 0.001f)
    {
        fprintf(stderr, "FAIL: %s actual=%.3f expected=%.3f\n",
                name, (double)actual, (double)expected);
        exit(1);
    }
}

static uint8_t checksum(const uint8_t frame[WT101_UART_FRAME_SIZE])
{
    uint8_t sum = 0U;
    uint32_t index;
    for (index = 0U; index < WT101_UART_FRAME_SIZE - 1U; index++)
    {
        sum = (uint8_t)(sum + frame[index]);
    }
    return sum;
}

static void test_wt101_preserves_signed_yaw_data(void)
{
    float yaw = 0.0f;
    uint8_t positive[WT101_UART_FRAME_SIZE] =
        {0x55U, 0x53U, 0U, 0U, 0U, 0U, 0x00U, 0x40U, 0U, 0U, 0U};
    uint8_t negative[WT101_UART_FRAME_SIZE] =
        {0x55U, 0x53U, 0U, 0U, 0U, 0U, 0x00U, 0xC0U, 0U, 0U, 0U};

    positive[10] = checksum(positive);
    negative[10] = checksum(negative);

    if (WT101_UART_ParseYaw(positive, &yaw) == 0U)
    {
        fputs("FAIL: positive yaw frame rejected\n", stderr);
        exit(1);
    }
    fail_float("positive yaw", yaw, 90.0f);

    if (WT101_UART_ParseYaw(negative, &yaw) == 0U)
    {
        fputs("FAIL: negative yaw frame rejected\n", stderr);
        exit(1);
    }
    fail_float("negative yaw", yaw, -90.0f);

    negative[10]++;
    if (WT101_UART_ParseYaw(negative, &yaw) != 0U)
    {
        fputs("FAIL: invalid checksum accepted\n", stderr);
        exit(1);
    }
}

static void test_mulun_pid_preserves_original_parameters_and_flow(void)
{
    PID_Mulun_HandleTypeDef pid;
    float output;

    PID_Mulun_Init(&pid);
    fail_float("P", pid.p, 12.0f);
    fail_float("I", pid.i, 2.0f);
    fail_float("D", pid.d, 2.0f);
    fail_float("integral max", pid.integral_max, 35.0f);
    fail_float("output max", pid.output_max, 350.0f);

    output = PID_Mulun_Calc(&pid, 10.0f, 0.0f);
    fail_float("first PID output", output, 160.0f);
    fail_float("first integral", pid.integral, 10.0f);
    fail_float("first last error", pid.last_error, 10.0f);

    output = PID_Mulun_Calc(&pid, 10.0f, 0.0f);
    fail_float("second PID output", output, 160.0f);
    fail_float("second integral", pid.integral, 20.0f);

    PID_Mulun_Init(&pid);
    output = PID_Mulun_Calc(&pid, 10.0f, 350.0f);
    fail_float("wrapped heading PID output", output, 320.0f);

    PID_Mulun_Init(&pid);
    output = PID_Mulun_Calc(&pid, 100.0f, 0.0f);
    fail_float("limited PID output", output, 350.0f);
    fail_float("integral separation", pid.integral, 0.0f);
}

int main(void)
{
    test_wt101_preserves_signed_yaw_data();
    test_mulun_pid_preserves_original_parameters_and_flow();
    puts("PASS: WT101 conversion and original Mulun PID calculations");
    return 0;
}
