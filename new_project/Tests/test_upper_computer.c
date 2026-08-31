/**
 ******************************************************************************
 * @file    test_upper_computer.c
 * @brief   Host regression test for the original upper-computer UART packet
 * @pin_resources PA2=USART2_TX and PA3=USART2_RX on target; none on host.
 * @peripherals Models the USART2 receive-complete callback.
 * @function Feeds exact bytes and verifies all ten packet fields and commands.
 * @purpose Prevents packet layout, values or receive flow from changing.
 ******************************************************************************
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "UART.h"
#include "UpperComputer.h"

GPIO_TypeDef test_gpioa = {1U};
GPIO_TypeDef test_gpiob = {2U};
uint32_t test_usart2_instance;
uint32_t test_usart1_instance;
uint32_t test_tim1_instance;
uint32_t test_tim2_instance;
UART_HandleTypeDef huart2 = {0};
static uint8_t *armed_byte;

void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init)
{
    (void)port;
    (void)init;
}

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pins, GPIO_PinState state)
{
    (void)port;
    (void)pins;
    (void)state;
}

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart)
{
    (void)huart;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart,
                                     uint8_t *data,
                                     uint16_t length)
{
    (void)huart;
    if (length != 1U)
    {
        return HAL_ERROR;
    }
    armed_byte = data;
    return HAL_OK;
}

static void feed(const uint8_t *bytes, uint32_t length)
{
    uint32_t index;
    for (index = 0U; index < length; index++)
    {
        if (armed_byte == NULL)
        {
            fputs("FAIL: USART2 receive not armed\n", stderr);
            exit(1);
        }
        *armed_byte = bytes[index];
        armed_byte = NULL;
        HAL_UART_RxCpltCallback(&huart2);
    }
}

int main(void)
{
    const uint8_t packet[UART_PACKET_LENGTH] =
        {0xB3U, 11U, 22U, 33U, 44U, 55U, 66U, 77U, 88U, 0xB4U};
    const UART_Packet_t *parsed;

    huart2.Instance = USART2;
    UART_Enable_Receive();
    feed(packet, UART_PACKET_LENGTH);
    if (rx_complete_flag == 0U)
    {
        fputs("FAIL: valid ten-byte packet not completed\n", stderr);
        return 1;
    }

    UART_Parse_Data();
    UART_Launch();
    parsed = UART_GetLatestPacket();

    if ((parsed->header != 0xB3U) ||
        (parsed->forward_speed != 11U) ||
        (parsed->horizontal_speed != 22U) ||
        (parsed->target_angle != 33U) ||
        (parsed->rudder_angle != 44U) ||
        (parsed->lift_rod != 55U) ||
        (parsed->horizontal_rod != 66U) ||
        (parsed->switch_one != 77U) ||
        (parsed->switch_two != 88U) ||
        (parsed->footer != 0xB4U))
    {
        fputs("FAIL: original UART packet fields changed\n", stderr);
        return 1;
    }

    if ((motor_vx != 11U) || (motor_vy != 22U) || (target_yaw != 33.0f))
    {
        fputs("FAIL: original chassis command assignment changed\n", stderr);
        return 1;
    }

    puts("PASS: original ten-byte upper-computer UART data flow");
    return 0;
}
