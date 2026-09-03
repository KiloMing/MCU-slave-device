/**
 ******************************************************************************
 * @file    test_buleteethtest.c
 * @brief   Host tests for Bluetooth four-wheel closed-loop chassis control
 * @pin_resources No physical pins; models USART2, PC13 and four encoders.
 * @peripherals Models USART2, GPIO, SysTick and four motor PID channels.
 * @function Verifies joystick parsing, Mecanum target mapping and PID updates.
 * @purpose Protects Bluetooth-to-four-wheel speed-loop behavior on the host.
 ******************************************************************************
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buleteethtest.h"
#include "MotorEncoder.h"
#include "MotorSpeedPID.h"
#include "UART.h"

static UART_RxByteHandler_t receive_handler;
static uint32_t fake_tick;
static uint32_t uart_baud;
static uint32_t motor_init_count;
static uint32_t encoder_init_count;
static uint32_t pid_init_count;
static uint32_t pid_run_count;
static uint32_t stop_count;
static uint32_t led_init_count;
static uint32_t led_write_count;
static GPIO_PinState led_state;
static int32_t configured_kp[4];
static int32_t configured_ki[4];
static int32_t configured_kd[4];
static int32_t configured_divisor[4];
static int32_t configured_feedforward[4];
static int32_t last_targets[4];
static int32_t last_measured[4];
static int32_t fake_measured[4] = {100, 200, 300, 400};
static char last_text[96];

GPIO_TypeDef test_gpioa = {1U};
GPIO_TypeDef test_gpiob = {2U};
GPIO_TypeDef test_gpioc = {3U};
uint32_t test_tim1_instance;
uint32_t test_tim2_instance;
uint32_t test_usart2_instance;
uint32_t test_usart1_instance;

void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init)
{
    if ((port == GPIOC) && (init->Pin == GPIO_PIN_13) &&
        (init->Mode == GPIO_MODE_OUTPUT_PP))
    {
        led_init_count++;
    }
}

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    if ((port == GPIOC) && (pin == GPIO_PIN_13))
    {
        led_state = state;
        led_write_count++;
    }
}

uint32_t HAL_GetTick(void)
{
    return fake_tick;
}

void UART_Init(uint32_t baud_rate)
{
    uart_baud = baud_rate;
}

void UART_StartReceiveIT(UART_RxByteHandler_t handler)
{
    receive_handler = handler;
}

void UART_SendString(const char *text)
{
    (void)snprintf(last_text, sizeof(last_text), "%s", text);
}

void Motor_Init(void)
{
    motor_init_count++;
}

void motor_stop_all(void)
{
    stop_count++;
}

void MotorEncoder_Init(void)
{
    encoder_init_count++;
}

void MotorEncoder_Sample(int32_t pulses[4], int32_t speeds[4])
{
    uint32_t index;

    for (index = 0U; index < 4U; index++)
    {
        pulses[index] = 0;
        speeds[index] = fake_measured[index];
    }
}

void MotorSpeedPID_Init(void)
{
    pid_init_count++;
}

void MotorSpeedPID_SetParameters(uint8_t motor_index,
                                 int32_t kp, int32_t ki, int32_t kd,
                                 int32_t gain_divisor,
                                 int32_t feedforward_pwm)
{
    configured_kp[motor_index] = kp;
    configured_ki[motor_index] = ki;
    configured_kd[motor_index] = kd;
    configured_divisor[motor_index] = gain_divisor;
    configured_feedforward[motor_index] = feedforward_pwm;
}

void MotorSpeedPID_SetTargets(const int32_t targets[4])
{
    (void)memcpy(last_targets, targets, sizeof(last_targets));
}

void MotorSpeedPID_SetMeasuredSpeeds(const int32_t measured[4])
{
    (void)memcpy(last_measured, measured, sizeof(last_measured));
}

uint8_t MotorSpeedPID_RunStep(void)
{
    pid_run_count++;
    return 1U;
}

static void fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
}

static void expect_i32(int32_t actual, int32_t expected, const char *message)
{
    if (actual != expected)
    {
        fprintf(stderr, "FAIL: %s actual=%ld expected=%ld\n", message,
                (long)actual, (long)expected);
        exit(1);
    }
}

static void expect_four(const int32_t actual[4],
                        int32_t a, int32_t b, int32_t c, int32_t d,
                        const char *message)
{
    if ((actual[0] != a) || (actual[1] != b) ||
        (actual[2] != c) || (actual[3] != d))
    {
        fprintf(stderr, "FAIL: %s actual=%ld,%ld,%ld,%ld\n", message,
                (long)actual[0], (long)actual[1],
                (long)actual[2], (long)actual[3]);
        exit(1);
    }
}

static void expect_float(float actual, float expected, const char *message)
{
    if (fabsf(actual - expected) > 0.001f)
    {
        fprintf(stderr, "FAIL: %s actual=%.3f expected=%.3f\n", message,
                (double)actual, (double)expected);
        exit(1);
    }
}

static void feed_packet(const char *packet)
{
    while (*packet != '\0')
    {
        receive_handler((uint8_t)*packet);
        packet++;
    }
}

static void test_packets_and_joystick_axes_remain_compatible(void)
{
    BluetoothTest_Joystick_t joystick;
    int32_t vx;
    int32_t vy;

    if (BluetoothTest_ParseJoystick("[j,-25,0,40,-80]", &joystick) == 0U)
    {
        fail("short joystick packet rejected");
    }
    expect_i32(joystick.left_x, -25, "left X");
    expect_i32(joystick.right_y, -80, "right Y");
    BluetoothTest_MapTranslation(&joystick, &vx, &vy);
    expect_i32(vx, -560, "right Y maps to backward vx");
    expect_i32(vy, -280, "right X maps to rightward vy");
    expect_float(BluetoothTest_MapRotation(-25), -175.0f,
                 "left X maps to rotation");

    if (BluetoothTest_ParseJoystick("[joystick,1,2,3,4]", &joystick) == 0U)
    {
        fail("long joystick packet rejected");
    }
    if (BluetoothTest_ParseJoystick("[x,1,2,3,4]", &joystick) != 0U)
    {
        fail("packet with wrong name accepted");
    }
}

static void test_mecanum_commands_map_to_normalized_speed_targets(void)
{
    int32_t targets[4];

    BluetoothTest_CalculateWheelTargets(700, 0, 0.0f, targets);
    expect_four(targets, 1600, 1600, 1600, 1600,
                "full forward targets");

    BluetoothTest_CalculateWheelTargets(0, -700, 0.0f, targets);
    expect_four(targets, 1600, -1600, -1600, 1600,
                "full right strafe targets");

    BluetoothTest_CalculateWheelTargets(0, 0, 700.0f, targets);
    expect_four(targets, 1600, 1600, -1600, -1600,
                "full rotation targets");

    BluetoothTest_CalculateWheelTargets(700, 700, 0.0f, targets);
    expect_four(targets, 0, 1600, 1600, 0,
                "diagonal command is normalized without clipping");
}

static void test_closed_loop_initialization_and_target_ramp(void)
{
    uint32_t index;

    fake_tick = 10U;
    BluetoothTest_Init();
    if ((uart_baud != 115200U) || (motor_init_count != 1U) ||
        (encoder_init_count != 1U) || (pid_init_count != 1U) ||
        (receive_handler == NULL) || (led_init_count != 1U))
    {
        fail("Bluetooth closed-loop initialization is incomplete");
    }

    expect_i32(configured_kp[0], 30, "Motor A Kp");
    expect_i32(configured_kp[1], 23, "Motor B Kp");
    expect_i32(configured_kp[2], 30, "Motor C Kp");
    expect_i32(configured_kp[3], 30, "Motor D Kp");
    for (index = 0U; index < 4U; index++)
    {
        expect_i32(configured_ki[index], 20, "motor Ki");
        expect_i32(configured_kd[index], 0, "motor Kd");
        expect_i32(configured_divisor[index], 100, "gain divisor");
        expect_i32(configured_feedforward[index], 0,
                   "dynamic Bluetooth target disables fixed feedforward");
    }

    feed_packet("[j,0,0,0,100]");
    BluetoothTest_RunStep();
    if (led_state != GPIO_PIN_SET)
    {
        fail("valid packet must toggle activity LED");
    }

    fake_tick = 20U;
    BluetoothTest_RunStep();
    expect_four(last_targets, 16, 16, 16, 16,
                "10 ms ramp advances four forward targets by 16");
    expect_four(last_measured, 100, 200, 300, 400,
                "all four measured speeds reach PID");
    expect_i32((int32_t)pid_run_count, 1, "PID runs after 10 ms");

    fake_tick = 1020U;
    BluetoothTest_RunStep();
    expect_four(last_targets, 1600, 1600, 1600, 1600,
                "ramp reaches common maximum target");

    feed_packet("[j,0,0,0,0]");
    BluetoothTest_RunStep();
    expect_four(last_targets, 0, 0, 0, 0,
                "center packet clears targets immediately");
    expect_i32((int32_t)stop_count, 1,
               "center packet immediately stops physical PWM");
}

int main(void)
{
    test_packets_and_joystick_axes_remain_compatible();
    test_mecanum_commands_map_to_normalized_speed_targets();
    test_closed_loop_initialization_and_target_ramp();
    puts("PASS: Bluetooth four-wheel closed-loop chassis control");
    return 0;
}
