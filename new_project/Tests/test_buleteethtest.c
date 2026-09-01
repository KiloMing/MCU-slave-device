/**
 ******************************************************************************
 * @file    test_buleteethtest.c
 * @brief   Host tests for Jiangxie Bluetooth joystick parsing and mapping
 * @pin_resources No physical pins are used by this host test.
 * @peripherals Models USART2 joystick data, SysTick and four-motor commands.
 * @function Verifies parsing, direct mapping, command execution and timeout.
 * @purpose Proves basic Bluetooth chassis motion before attitude control.
 ******************************************************************************
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "buleteethtest.h"
#include "UART.h"

static UART_RxByteHandler_t receive_handler;
static uint32_t fake_tick;
static uint32_t uart_baud;
static uint32_t motor_init_count;
static uint32_t stop_count;
static uint32_t move_count;
static int32_t last_vx;
static int32_t last_vy;
static float last_omega;
static int32_t last_pwm_limit;
static char last_text[96];
GPIO_TypeDef test_gpioa = {1U};
GPIO_TypeDef test_gpiob = {2U};
GPIO_TypeDef test_gpioc = {3U};
uint32_t test_tim1_instance;
uint32_t test_tim2_instance;
uint32_t test_usart2_instance;
uint32_t test_usart1_instance;
static uint32_t led_init_count;
static uint32_t led_write_count;
static GPIO_PinState led_state;

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

void mecanum_move(int32_t vx, int32_t vy, float omega)
{
    move_count++;
    last_vx = vx;
    last_vy = vy;
    last_omega = omega;
}

void mecanum_move_limited(int32_t vx, int32_t vy, float omega,
                          int32_t pwm_limit)
{
    move_count++;
    last_vx = vx;
    last_vy = vy;
    last_omega = omega;
    last_pwm_limit = pwm_limit;
}

uint32_t HAL_GetTick(void)
{
    return fake_tick;
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

static void expect_float(float actual, float expected, const char *message)
{
    if (fabsf(actual - expected) > 0.001f)
    {
        fprintf(stderr, "FAIL: %s actual=%.3f expected=%.3f\n", message,
                (double)actual, (double)expected);
        exit(1);
    }
}

static void test_short_and_long_packets_keep_signed_values(void)
{
    BluetoothTest_Joystick_t joystick;

    if (BluetoothTest_ParseJoystick("[j,-25,0,40,-80]", &joystick) == 0U)
    {
        fail("short joystick packet rejected");
    }
    expect_i32(joystick.left_x, -25, "short left X");
    expect_i32(joystick.left_y, 0, "short left Y");
    expect_i32(joystick.right_x, 40, "short right X");
    expect_i32(joystick.right_y, -80, "short right Y");

    if (BluetoothTest_ParseJoystick("[joystick,100,-100,-100,100]", &joystick) == 0U)
    {
        fail("long joystick packet rejected");
    }
    expect_i32(joystick.left_x, 100, "long left X");
    expect_i32(joystick.right_y, 100, "long right Y");

    if (BluetoothTest_ParseJoystick("[j,1,2,3]", &joystick) != 0U)
    {
        fail("packet with missing field accepted");
    }
    if (BluetoothTest_ParseJoystick("[x,1,2,3,4]", &joystick) != 0U)
    {
        fail("packet with wrong name accepted");
    }
}

static void test_right_joystick_maps_to_signed_360_degree_translation(void)
{
    BluetoothTest_Joystick_t joystick = {-25, 99, 40, -80};
    int32_t vx;
    int32_t vy;

    BluetoothTest_MapTranslation(&joystick, &vx, &vy);
    expect_i32(vx, -560, "right Y maps to backward vx at 70 percent scale");
    expect_i32(vy, -280, "right X maps to rightward vy at 70 percent scale");

    joystick.right_x = -100;
    joystick.right_y = 100;
    BluetoothTest_MapTranslation(&joystick, &vx, &vy);
    expect_i32(vx, 700, "maximum forward command uses 70 percent scale");
    expect_i32(vy, 700, "maximum left command uses 70 percent scale");
}

static void test_left_x_maps_directly_to_signed_rotation(void)
{
    expect_float(BluetoothTest_MapRotation(100), 700.0f,
                 "left X positive rotation");
    expect_float(BluetoothTest_MapRotation(-100), -700.0f,
                 "left X negative rotation");
    expect_float(BluetoothTest_MapRotation(0), 0.0f,
                 "centered left X stops rotation");
}

static void feed_packet(const char *packet)
{
    while (*packet != '\0')
    {
        receive_handler((uint8_t)*packet);
        packet++;
    }
}

static void test_open_loop_command_led_and_immediate_center_stop(void)
{
    fake_tick = 10U;
    BluetoothTest_Init();
    if ((uart_baud != 115200U) || (motor_init_count != 1U) ||
        (receive_handler == NULL) || (led_init_count != 1U) ||
        (led_state != GPIO_PIN_RESET))
    {
        fail("Bluetooth test initialization is incomplete");
    }

    feed_packet("[j,-25,99,40,-80]");
    BluetoothTest_RunStep();
    expect_i32((int32_t)move_count, 1, "valid packet drives chassis once");
    expect_i32(last_vx, -560, "run step backward vx");
    expect_i32(last_vy, -280, "run step rightward vy");
    expect_float(last_omega, -175.0f, "run step direct rotation");
    expect_i32(last_pwm_limit, 699,
               "Bluetooth final wheel PWM is capped below 70 percent");
    expect_i32((int32_t)led_write_count, 2,
               "valid packet toggles PC13 after initial off write");
    if (led_state != GPIO_PIN_SET)
    {
        fail("first valid packet must light PC13 LED");
    }

    fake_tick = 511U;
    BluetoothTest_RunStep();
    expect_i32((int32_t)stop_count, 0,
               "missing packets must not trigger delayed stop");

    feed_packet("[x,0,0,0,0]");
    BluetoothTest_RunStep();
    expect_i32((int32_t)led_write_count, 2,
               "invalid packet must not toggle activity LED");

    feed_packet("[j,0,0,0,0]");
    BluetoothTest_RunStep();
    expect_i32(last_vx, 0, "center packet stops vx immediately");
    expect_i32(last_vy, 0, "center packet stops vy immediately");
    expect_float(last_omega, 0.0f, "center packet stops rotation immediately");
    if ((led_write_count != 3U) || (led_state != GPIO_PIN_RESET))
    {
        fail("second valid packet must toggle PC13 LED off");
    }
}

int main(void)
{
    test_short_and_long_packets_keep_signed_values();
    test_right_joystick_maps_to_signed_360_degree_translation();
    test_left_x_maps_directly_to_signed_rotation();
    test_open_loop_command_led_and_immediate_center_stop();
    puts("PASS: Bluetooth joystick parsing and open-loop chassis mapping");
    return 0;
}
