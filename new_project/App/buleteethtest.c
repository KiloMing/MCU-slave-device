/**
 ******************************************************************************
 * @file    buleteethtest.c
 * @brief   Jiangxie Bluetooth four-wheel speed PID Mecanum chassis test
 * @pin_resources
 *   - HC-08: PA2=USART2_TX, PA3=USART2_RX, common GND.
 *   - Activity LED: PC13 -> 510 ohm -> LED anode; cathode -> GND.
 *   - Motor PWM: PA0, PA1, PA8 and PA11.
 *   - Motor direction: PA4, PB3/PB4/PB5/PB12/PB13/PB14/PB15.
 *   - A encoder: PB6/PB7; B encoder: PA6/PA7.
 *   - C encoder: A phase PB1, B phase PB0.
 *   - D encoder: A phase PA12, B phase PA5.
 * @peripherals USART2, TIM1-TIM4, GPIOA/B/C, AFIO, EXTI and SysTick.
 * @function Converts [j,LX,LY,RX,RY] into four ramped speed PID targets.
 * @purpose Controls 360-degree translation and rotation without WT101.
 * @migration Retains the original joystick axes and Mecanum wheel equations.
 *   Standalone calibration records: A=1600/525/0.30/0.20,
 *   B=2500/465/0.23/0.20, C=1600/525/0.30/0.20,
 *   D=1600/525/0.30/0.20 (target/feedforward/Kp/Ki; Kd=0).
 ******************************************************************************
 */

#include "buleteethtest.h"

#include "Motor.h"
#include "MotorEncoder.h"
#include "MotorSpeedPID.h"
#include "UART.h"
#include "stm32f1xx_hal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLUETOOTH_PACKET_BUFFER_SIZE 64U
#define BLUETOOTH_JOYSTICK_LIMIT     100L
#define BLUETOOTH_TRANSLATION_SCALE  7L
#define BLUETOOTH_ROTATION_SCALE     7.0f
#define BLUETOOTH_AXIS_OUTPUT_LIMIT  700L
#define BLUETOOTH_SPEED_LIMIT        1600L
#define BLUETOOTH_RAMP_TIME_MS       1000U
#define BLUETOOTH_CONTROL_PERIOD_MS  10U
#define BLUETOOTH_TELEMETRY_MS       200U
#define BLUETOOTH_PID_GAIN_DIVISOR   100
#define BLUETOOTH_PID_KI             20
#define BLUETOOTH_PID_KD             0

static volatile char bluetooth_packet[BLUETOOTH_PACKET_BUFFER_SIZE];
static volatile uint8_t bluetooth_packet_index;
static volatile uint8_t bluetooth_collecting;
static volatile uint8_t bluetooth_packet_ready;
static BluetoothTest_Joystick_t bluetooth_joystick;
static uint8_t bluetooth_command_valid;
static GPIO_PinState bluetooth_led_state;
static uint32_t bluetooth_last_control_tick;
static uint32_t bluetooth_ramp_remainder;
static uint32_t bluetooth_last_telemetry_tick;
static int32_t bluetooth_command_vx;
static int32_t bluetooth_command_vy;
static float bluetooth_command_omega;
static int32_t bluetooth_desired_targets[MOTOR_SPEED_PID_MOTOR_COUNT];
static int32_t bluetooth_ramped_targets[MOTOR_SPEED_PID_MOTOR_COUNT];
static int32_t bluetooth_encoder_pulses[MOTOR_ENCODER_COUNT];
static int32_t bluetooth_measured_speeds[MOTOR_ENCODER_COUNT];

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

static int32_t BluetoothTest_Abs(int32_t value)
{
    return (value < 0) ? -value : value;
}

static int32_t BluetoothTest_RampValue(int32_t current, int32_t target,
                                       int32_t max_step)
{
    int32_t difference = target - current;

    if (difference > max_step)
    {
        return current + max_step;
    }
    if (difference < -max_step)
    {
        return current - max_step;
    }
    return target;
}

static uint8_t BluetoothTest_TargetsAreZero(const int32_t targets[4])
{
    return (uint8_t)((targets[0] == 0) && (targets[1] == 0) &&
                     (targets[2] == 0) && (targets[3] == 0));
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

void BluetoothTest_CalculateWheelTargets(int32_t vx, int32_t vy, float omega,
                                         int32_t targets[4])
{
    int32_t raw[4];
    int32_t omega_integer;
    int32_t normalization;
    uint32_t index;

    if (targets == NULL)
    {
        return;
    }

    omega_integer = (int32_t)omega;
    raw[0] = vx - vy + omega_integer;
    raw[1] = vx + vy + omega_integer;
    raw[2] = vx + vy - omega_integer;
    raw[3] = vx - vy - omega_integer;
    normalization = BLUETOOTH_AXIS_OUTPUT_LIMIT;

    for (index = 0U; index < 4U; index++)
    {
        int32_t magnitude = BluetoothTest_Abs(raw[index]);
        if (magnitude > normalization)
        {
            normalization = magnitude;
        }
    }

    for (index = 0U; index < 4U; index++)
    {
        targets[index] = (int32_t)(((int64_t)raw[index] *
                                    BLUETOOTH_SPEED_LIMIT) /
                                   normalization);
    }
}

void BluetoothTest_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    const int32_t stopped_targets[4] = {0, 0, 0, 0};
    uint32_t index;

    UART_Init(115200U);
    Motor_Init();
    MotorEncoder_Init();
    MotorSpeedPID_Init();
    MotorSpeedPID_SetParameters(0U, 30, BLUETOOTH_PID_KI,
                                BLUETOOTH_PID_KD,
                                BLUETOOTH_PID_GAIN_DIVISOR, 0);
    MotorSpeedPID_SetParameters(1U, 23, BLUETOOTH_PID_KI,
                                BLUETOOTH_PID_KD,
                                BLUETOOTH_PID_GAIN_DIVISOR, 0);
    MotorSpeedPID_SetParameters(2U, 30, BLUETOOTH_PID_KI,
                                BLUETOOTH_PID_KD,
                                BLUETOOTH_PID_GAIN_DIVISOR, 0);
    MotorSpeedPID_SetParameters(3U, 30, BLUETOOTH_PID_KI,
                                BLUETOOTH_PID_KD,
                                BLUETOOTH_PID_GAIN_DIVISOR, 0);
    MotorSpeedPID_SetTargets(stopped_targets);

    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &gpio);
    bluetooth_led_state = GPIO_PIN_RESET;
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, bluetooth_led_state);

    bluetooth_packet_index = 0U;
    bluetooth_collecting = 0U;
    bluetooth_packet_ready = 0U;
    bluetooth_command_valid = 0U;
    bluetooth_command_vx = 0;
    bluetooth_command_vy = 0;
    bluetooth_command_omega = 0.0f;
    bluetooth_ramp_remainder = 0U;
    for (index = 0U; index < 4U; index++)
    {
        bluetooth_desired_targets[index] = 0;
        bluetooth_ramped_targets[index] = 0;
        bluetooth_encoder_pulses[index] = 0;
        bluetooth_measured_speeds[index] = 0;
    }
    bluetooth_last_control_tick = HAL_GetTick();
    bluetooth_last_telemetry_tick = HAL_GetTick();
    UART_StartReceiveIT(BluetoothTest_ReceiveByte);
    UART_SendString("BLUETOOTH FOUR-WHEEL PID TEST READY\r\n");
}

void BluetoothTest_RunStep(void)
{
    char packet_copy[BLUETOOTH_PACKET_BUFFER_SIZE];
    char telemetry[96];
    uint32_t now;
    uint32_t elapsed;
    uint32_t ramp_numerator;
    int32_t max_step;
    uint32_t index;

    if (bluetooth_packet_ready != 0U)
    {
        (void)strncpy(packet_copy, (const char *)bluetooth_packet,
                      sizeof(packet_copy) - 1U);
        packet_copy[sizeof(packet_copy) - 1U] = '\0';
        bluetooth_packet_ready = 0U;
        if (BluetoothTest_ParseJoystick(packet_copy, &bluetooth_joystick) != 0U)
        {
            BluetoothTest_MapTranslation(&bluetooth_joystick,
                                         &bluetooth_command_vx,
                                         &bluetooth_command_vy);
            bluetooth_command_omega =
                BluetoothTest_MapRotation(bluetooth_joystick.left_x);
            BluetoothTest_CalculateWheelTargets(bluetooth_command_vx,
                                                bluetooth_command_vy,
                                                bluetooth_command_omega,
                                                bluetooth_desired_targets);
            bluetooth_command_valid = 1U;
            bluetooth_led_state = (bluetooth_led_state == GPIO_PIN_RESET) ?
                                  GPIO_PIN_SET : GPIO_PIN_RESET;
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, bluetooth_led_state);

            if (BluetoothTest_TargetsAreZero(bluetooth_desired_targets) != 0U)
            {
                for (index = 0U; index < 4U; index++)
                {
                    bluetooth_ramped_targets[index] = 0;
                }
                bluetooth_ramp_remainder = 0U;
                MotorSpeedPID_SetTargets(bluetooth_ramped_targets);
                motor_stop_all();
            }
        }
    }

    now = HAL_GetTick();
    if ((bluetooth_command_valid == 0U) ||
        ((now - bluetooth_last_control_tick) < BLUETOOTH_CONTROL_PERIOD_MS))
    {
        return;
    }

    elapsed = now - bluetooth_last_control_tick;
    if (elapsed > BLUETOOTH_RAMP_TIME_MS)
    {
        elapsed = BLUETOOTH_RAMP_TIME_MS;
    }
    bluetooth_last_control_tick = now;
    ramp_numerator = bluetooth_ramp_remainder +
                     ((uint32_t)BLUETOOTH_SPEED_LIMIT * elapsed);
    max_step = (int32_t)(ramp_numerator / BLUETOOTH_RAMP_TIME_MS);
    bluetooth_ramp_remainder = ramp_numerator % BLUETOOTH_RAMP_TIME_MS;

    for (index = 0U; index < 4U; index++)
    {
        bluetooth_ramped_targets[index] =
            BluetoothTest_RampValue(bluetooth_ramped_targets[index],
                                    bluetooth_desired_targets[index],
                                    max_step);
    }

    MotorEncoder_Sample(bluetooth_encoder_pulses,
                        bluetooth_measured_speeds);
    MotorSpeedPID_SetTargets(bluetooth_ramped_targets);
    MotorSpeedPID_SetMeasuredSpeeds(bluetooth_measured_speeds);
    (void)MotorSpeedPID_RunStep();

    if ((now - bluetooth_last_telemetry_tick) >= BLUETOOTH_TELEMETRY_MS)
    {
        (void)snprintf(telemetry, sizeof(telemetry),
                       "[BT,%ld,%ld,%ld]\r\n",
                       (long)bluetooth_command_vx,
                       (long)bluetooth_command_vy,
                       (long)bluetooth_command_omega);
        UART_SendString(telemetry);
        bluetooth_last_telemetry_tick = now;
    }
}
