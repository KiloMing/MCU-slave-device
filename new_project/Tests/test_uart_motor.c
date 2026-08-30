/**
 ******************************************************************************
 * @file    test_uart_motor.c
 * @brief   Host behavior tests for the migrated UART and motor drivers
 *
 * @pin_resources
 *   - UART: PA2 TX, PA3 RX, PA6 RS485 DE, PA7 RS485 /RE.
 *   - Motor PWM: PA0, PA1, PA8, PA11.
 *   - Motor direction: PA4, PB3, PB4, PB5, PB12, PB13, PB14, PB15.
 *
 * @peripherals
 *   - USART2, TIM1 CH1/CH4, TIM2 CH1/CH2 and GPIO.
 *
 * @function
 *   - Verifies observable UART traffic, motor direction and PWM behavior.
 *
 * @purpose
 *   - Prevents pin, direction, channel, duty-cycle and protocol regressions.
 ******************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Motor.h"
#include "UART.h"

GPIO_TypeDef test_gpioa = {1U};
GPIO_TypeDef test_gpiob = {2U};
uint32_t test_tim1_instance;
uint32_t test_tim2_instance;
uint32_t test_usart2_instance;

static GPIO_PinState gpioa_state[16];
static GPIO_PinState gpiob_state[16];
static uint32_t tim1_compare[5];
static uint32_t tim2_compare[5];
static uint32_t tim1_started;
static uint32_t tim2_started;
static UART_HandleTypeDef uart_init_snapshot;
static uint32_t uart_init_count;
static uint8_t uart_tx[128];
static size_t uart_tx_length;
static uint8_t uart_rx_value;
typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pins;
    uint32_t mode;
} GpioInitRecord;
static GpioInitRecord gpio_init_records[16];
static size_t gpio_init_count;
static uint32_t swj_nojtag_count;

static void fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
}

static void expect_u32(uint32_t actual, uint32_t expected, const char *message)
{
    if (actual != expected)
    {
        fprintf(stderr, "FAIL: %s (actual=%lu expected=%lu)\n", message,
                (unsigned long)actual, (unsigned long)expected);
        exit(1);
    }
}

static unsigned int pin_index(uint16_t pin)
{
    unsigned int index;
    for (index = 0U; index < 16U; index++)
    {
        if (pin == (uint16_t)(1U << index))
        {
            return index;
        }
    }
    fail("invalid single GPIO pin");
    return 0U;
}

static GPIO_PinState read_pin(GPIO_TypeDef *port, uint16_t pin)
{
    return (port == GPIOA) ? gpioa_state[pin_index(pin)]
                           : gpiob_state[pin_index(pin)];
}

static int gpio_was_initialized(GPIO_TypeDef *port, uint16_t pins, uint32_t mode)
{
    size_t index;
    for (index = 0U; index < gpio_init_count; index++)
    {
        if ((gpio_init_records[index].port == port) &&
            (gpio_init_records[index].pins == pins) &&
            (gpio_init_records[index].mode == mode))
        {
            return 1;
        }
    }
    return 0;
}

void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init)
{
    if (gpio_init_count >= (sizeof(gpio_init_records) / sizeof(gpio_init_records[0])))
    {
        fail("too many GPIO initialization calls");
    }
    gpio_init_records[gpio_init_count].port = port;
    gpio_init_records[gpio_init_count].pins = init->Pin;
    gpio_init_records[gpio_init_count].mode = init->Mode;
    gpio_init_count++;
}

void Test_HAL_SwjNoJtag(void)
{
    swj_nojtag_count++;
}

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pins, GPIO_PinState state)
{
    unsigned int index;
    for (index = 0U; index < 16U; index++)
    {
        uint16_t pin = (uint16_t)(1U << index);
        if ((pins & pin) != 0U)
        {
            if (port == GPIOA)
            {
                gpioa_state[index] = state;
            }
            else
            {
                gpiob_state[index] = state;
            }
        }
    }
}

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart)
{
    uart_init_snapshot = *huart;
    uart_init_count++;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *data,
                                   uint16_t length, uint32_t timeout)
{
    (void)huart;
    (void)timeout;
    if ((uart_tx_length + length) > sizeof(uart_tx))
    {
        return HAL_ERROR;
    }
    memcpy(&uart_tx[uart_tx_length], data, length);
    uart_tx_length += length;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *data,
                                  uint16_t length, uint32_t timeout)
{
    (void)huart;
    (void)timeout;
    if (length != 1U)
    {
        return HAL_ERROR;
    }
    *data = uart_rx_value;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef *htim)
{
    (void)htim;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIMEx_MasterConfigSynchronization(TIM_HandleTypeDef *htim,
                                                        TIM_MasterConfigTypeDef *config)
{
    (void)htim;
    (void)config;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef *htim,
                                            TIM_OC_InitTypeDef *config,
                                            uint32_t channel)
{
    (void)htim;
    (void)config;
    (void)channel;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t channel)
{
    if (htim->Instance == TIM1)
    {
        tim1_started |= (1UL << channel);
    }
    else
    {
        tim2_started |= (1UL << channel);
    }
    return HAL_OK;
}

void Test_HAL_SetCompare(TIM_HandleTypeDef *htim, uint32_t channel, uint32_t value)
{
    if (htim->Instance == TIM1)
    {
        tim1_compare[channel] = value;
    }
    else
    {
        tim2_compare[channel] = value;
    }
}

static void test_uart_uses_verified_ttl_configuration(void)
{
    UART_Init(115200U);

    expect_u32(uart_init_count, 1U, "USART2 initialized once");
    if (uart_init_snapshot.Instance != USART2)
    {
        fail("UART must use USART2");
    }
    expect_u32(uart_init_snapshot.Init.BaudRate, 115200U, "baud rate unchanged");
    expect_u32(uart_init_snapshot.Init.WordLength, UART_WORDLENGTH_8B, "8 data bits");
    expect_u32(uart_init_snapshot.Init.StopBits, UART_STOPBITS_1, "one stop bit");
    expect_u32(uart_init_snapshot.Init.Parity, UART_PARITY_NONE, "no parity");
    expect_u32(read_pin(GPIOA, GPIO_PIN_6), GPIO_PIN_RESET, "RS485 DE disabled");
    expect_u32(read_pin(GPIOA, GPIO_PIN_7), GPIO_PIN_SET, "RS485 receiver output disabled");
    if (!gpio_was_initialized(GPIOA, GPIO_PIN_2, GPIO_MODE_AF_PP))
    {
        fail("PA2 must be configured as USART2 alternate-function output");
    }
    if (!gpio_was_initialized(GPIOA, GPIO_PIN_3, GPIO_MODE_INPUT))
    {
        fail("PA3 must be configured as USART2 input");
    }
}

static void test_uart_sends_and_receives_original_bytes(void)
{
    uart_tx_length = 0U;
    UART_SendString("USART2 TTL TEST READY\r\n");
    if ((uart_tx_length != 23U) ||
        (memcmp(uart_tx, "USART2 TTL TEST READY\r\n", 23U) != 0))
    {
        fail("UART string bytes changed");
    }

    uart_rx_value = 0xA5U;
    expect_u32(UART_ReceiveByte(), 0xA5U, "received byte returned unchanged");
}

static void expect_forward_pins(void)
{
    expect_u32(read_pin(GPIOB, GPIO_PIN_5), GPIO_PIN_SET, "LF forward PB5");
    expect_u32(read_pin(GPIOB, GPIO_PIN_12), GPIO_PIN_RESET, "LF forward PB12");
    expect_u32(read_pin(GPIOB, GPIO_PIN_13), GPIO_PIN_SET, "LB forward PB13");
    expect_u32(read_pin(GPIOB, GPIO_PIN_14), GPIO_PIN_RESET, "LB forward PB14");
    expect_u32(read_pin(GPIOB, GPIO_PIN_15), GPIO_PIN_SET, "RF forward PB15");
    expect_u32(read_pin(GPIOA, GPIO_PIN_4), GPIO_PIN_RESET, "RF forward PA4");
    expect_u32(read_pin(GPIOB, GPIO_PIN_3), GPIO_PIN_SET, "RB forward PB3");
    expect_u32(read_pin(GPIOB, GPIO_PIN_4), GPIO_PIN_RESET, "RB forward PB4");
}

static void expect_reverse_pins(void)
{
    expect_u32(read_pin(GPIOB, GPIO_PIN_5), GPIO_PIN_RESET, "LF reverse PB5");
    expect_u32(read_pin(GPIOB, GPIO_PIN_12), GPIO_PIN_SET, "LF reverse PB12");
    expect_u32(read_pin(GPIOB, GPIO_PIN_13), GPIO_PIN_RESET, "LB reverse PB13");
    expect_u32(read_pin(GPIOB, GPIO_PIN_14), GPIO_PIN_SET, "LB reverse PB14");
    expect_u32(read_pin(GPIOB, GPIO_PIN_15), GPIO_PIN_RESET, "RF reverse PB15");
    expect_u32(read_pin(GPIOA, GPIO_PIN_4), GPIO_PIN_SET, "RF reverse PA4");
    expect_u32(read_pin(GPIOB, GPIO_PIN_3), GPIO_PIN_RESET, "RB reverse PB3");
    expect_u32(read_pin(GPIOB, GPIO_PIN_4), GPIO_PIN_SET, "RB reverse PB4");
}

static void expect_all_pwm(uint32_t value)
{
    expect_u32(tim2_compare[TIM_CHANNEL_1], value, "LF uses TIM2 CH1");
    expect_u32(tim2_compare[TIM_CHANNEL_2], value, "LB uses TIM2 CH2");
    expect_u32(tim1_compare[TIM_CHANNEL_1], value, "RF uses TIM1 CH1");
    expect_u32(tim1_compare[TIM_CHANNEL_4], value, "RB uses TIM1 CH4");
}

static void test_motor_preserves_four_wheel_test_behavior(void)
{
    Motor_Init();
    expect_u32(htim1.Init.Prescaler, 6U, "TIM1 prescaler unchanged");
    expect_u32(htim2.Init.Prescaler, 6U, "TIM2 prescaler unchanged");
    expect_u32(htim1.Init.Period, 999U, "TIM1 period unchanged");
    expect_u32(htim2.Init.Period, 999U, "TIM2 period unchanged");
    if (!gpio_was_initialized(GPIOA,
                              GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_11,
                              GPIO_MODE_AF_PP))
    {
        fail("all four PWM pins must use alternate-function push-pull mode");
    }
    expect_u32(swj_nojtag_count, 1U, "JTAG disabled once to release PB3/PB4");
    expect_u32((tim1_started & (1UL << TIM_CHANNEL_1)) != 0U, 1U, "RF PWM started");
    expect_u32((tim1_started & (1UL << TIM_CHANNEL_4)) != 0U, 1U, "RB PWM started");
    expect_u32((tim2_started & (1UL << TIM_CHANNEL_1)) != 0U, 1U, "LF PWM started");
    expect_u32((tim2_started & (1UL << TIM_CHANNEL_2)) != 0U, 1U, "LB PWM started");

    motor_all_set(500);
    expect_forward_pins();
    expect_all_pwm(500U);

    motor_all_set(-500);
    expect_reverse_pins();
    expect_all_pwm(500U);

    motor_all_set(0);
    expect_all_pwm(0U);

    motor_all_set(1200);
    expect_all_pwm(999U);
}

int main(void)
{
    test_uart_uses_verified_ttl_configuration();
    test_uart_sends_and_receives_original_bytes();
    test_motor_preserves_four_wheel_test_behavior();
    puts("PASS: UART and motor migration behavior");
    return 0;
}
