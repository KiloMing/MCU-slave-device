/**
 ******************************************************************************
 * @file    buleteethtest.c
 * @brief   Jiangxie Bluetooth joystick open-loop Mecanum chassis test
 * @pin_resources
 *   - HC-08: PA2=USART2_TX, PA3=USART2_RX, common GND.
 *   - Activity LED: PC13 -> 510 ohm -> LED anode; LED cathode -> GND.
 *   - Motor PWM: PA0, PA1, PA8, PA11.
 *   - Motor direction: PA4, PB3, PB4, PB5, PB12, PB13, PB14, PB15.
 * @peripherals USART2, TIM1, TIM2, GPIOA/GPIOB/GPIOC, AFIO and SysTick.
 * @function Converts [j,LX,LY,RX,RY] directly into signed vx, vy and omega.
 * @purpose Verifies chassis motion without WT101, attitude or heading PID.
 * @migration The original mecanum_move() equations are not modified.
 ******************************************************************************
 */

#include "buleteethtest.h"

#include "Motor.h"
#include "UART.h"
#include "stm32f1xx_hal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLUETOOTH_PACKET_BUFFER_SIZE 64U
#define BLUETOOTH_JOYSTICK_LIMIT     100L
#define BLUETOOTH_TRANSLATION_SCALE  7L
#define BLUETOOTH_ROTATION_SCALE     7.0f
#define BLUETOOTH_PWM_LIMIT          699L
#define BLUETOOTH_TELEMETRY_MS       200U

static volatile char bluetooth_packet[BLUETOOTH_PACKET_BUFFER_SIZE];
static volatile uint8_t bluetooth_packet_index;
static volatile uint8_t bluetooth_collecting;
static volatile uint8_t bluetooth_packet_ready;
static BluetoothTest_Joystick_t bluetooth_joystick;
static uint8_t bluetooth_command_valid;
static GPIO_PinState bluetooth_led_state;
static uint32_t bluetooth_last_telemetry_tick;

static int32_t BluetoothTest_ClampJoystick(int32_t value)
{
    if (value > BLUETOOTH_JOYSTICK_LIMIT)
    {
        return BLUETOOTH_JOYSTICK_LIMIT;
    }
    if (value < -BLUETOOTH_JOYSTICK_LIMIT)
    {
        return -BLUETOOTH_JOYSTICK_LIMIT;
    }
    return value;
}

static void BluetoothTest_ReceiveByte(uint8_t byte)
{
    if (bluetooth_packet_ready != 0U)
    {
        return;
    }

    if (byte == (uint8_t)'[')
    {
        bluetooth_packet_index = 0U;
        bluetooth_collecting = 1U;
    }

    if (bluetooth_collecting == 0U)
    {
        return;
    }

    if (bluetooth_packet_index >= (BLUETOOTH_PACKET_BUFFER_SIZE - 1U))
    {
        bluetooth_packet_index = 0U;
        bluetooth_collecting = 0U;
        return;
    }

    bluetooth_packet[bluetooth_packet_index++] = (char)byte;
    if (byte == (uint8_t)']')
    {
        bluetooth_packet[bluetooth_packet_index] = '\0';
        bluetooth_collecting = 0U;
        bluetooth_packet_ready = 1U;
    }
}

uint8_t BluetoothTest_ParseJoystick(const char *packet,
                                    BluetoothTest_Joystick_t *joystick)
{
    const char *cursor;
    char *end;
    long values[4];
    uint32_t index;

    if ((packet == NULL) || (joystick == NULL))
    {
        return 0U;
    }

    if (strncmp(packet, "[j,", 3U) == 0)
    {
        cursor = packet + 3;
    }
    else if (strncmp(packet, "[joystick,", 10U) == 0)
    {
        cursor = packet + 10;
    }
    else
    {
        return 0U;
    }

    for (index = 0U; index < 4U; index++)
    {
        values[index] = strtol(cursor, &end, 10);
        if (end == cursor)
        {
            return 0U;
        }
        if (index < 3U)
        {
            if (*end != ',')
            {
                return 0U;
            }
            cursor = end + 1;
        }
        else if ((end[0] != ']') || (end[1] != '\0'))
        {
            return 0U;
        }
    }

    joystick->left_x = BluetoothTest_ClampJoystick((int32_t)values[0]);
    joystick->left_y = BluetoothTest_ClampJoystick((int32_t)values[1]);
    joystick->right_x = BluetoothTest_ClampJoystick((int32_t)values[2]);
    joystick->right_y = BluetoothTest_ClampJoystick((int32_t)values[3]);
    return 1U;
}

void BluetoothTest_MapTranslation(const BluetoothTest_Joystick_t *joystick,
                                  int32_t *vx, int32_t *vy)
{
    if ((joystick == NULL) || (vx == NULL) || (vy == NULL))
    {
        return;
    }

    *vx = BluetoothTest_ClampJoystick(joystick->right_y) *
          BLUETOOTH_TRANSLATION_SCALE;
    *vy = -BluetoothTest_ClampJoystick(joystick->right_x) *
          BLUETOOTH_TRANSLATION_SCALE;
}

float BluetoothTest_MapRotation(int32_t left_x)
{
    left_x = BluetoothTest_ClampJoystick(left_x);
    return (float)left_x * BLUETOOTH_ROTATION_SCALE;
}

void BluetoothTest_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    UART_Init(115200U);
    Motor_Init();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    bluetooth_led_state = GPIO_PIN_RESET;
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, bluetooth_led_state);
    bluetooth_packet_index = 0U;
    bluetooth_collecting = 0U;
    bluetooth_packet_ready = 0U;
    bluetooth_command_valid = 0U;
    bluetooth_last_telemetry_tick = HAL_GetTick();
    UART_StartReceiveIT(BluetoothTest_ReceiveByte);
    UART_SendString("BLUETOOTH OPEN LOOP CHASSIS TEST READY\r\n");
}

void BluetoothTest_RunStep(void)
{
    char packet_copy[BLUETOOTH_PACKET_BUFFER_SIZE];
    char telemetry[96];
    float omega;
    int32_t vx;
    int32_t vy;
    uint32_t now;

    if (bluetooth_packet_ready != 0U)
    {
        (void)strncpy(packet_copy, (const char *)bluetooth_packet,
                      sizeof(packet_copy) - 1U);
        packet_copy[sizeof(packet_copy) - 1U] = '\0';
        bluetooth_packet_ready = 0U;
        if (BluetoothTest_ParseJoystick(packet_copy, &bluetooth_joystick) != 0U)
        {
            bluetooth_command_valid = 1U;
            bluetooth_led_state = (bluetooth_led_state == GPIO_PIN_RESET) ?
                                  GPIO_PIN_SET : GPIO_PIN_RESET;
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, bluetooth_led_state);
        }
    }

    now = HAL_GetTick();
    if (bluetooth_command_valid == 0U)
    {
        return;
    }

    BluetoothTest_MapTranslation(&bluetooth_joystick, &vx, &vy);
    omega = BluetoothTest_MapRotation(bluetooth_joystick.left_x);
    mecanum_move_limited(vx, vy, omega, BLUETOOTH_PWM_LIMIT);

    if ((now - bluetooth_last_telemetry_tick) >= BLUETOOTH_TELEMETRY_MS)
    {
        (void)snprintf(telemetry, sizeof(telemetry),
                       "[BT,%ld,%ld,%ld]\r\n",
                       (long)vx, (long)vy, (long)omega);
        UART_SendString(telemetry);
        bluetooth_last_telemetry_tick = now;
    }
}
