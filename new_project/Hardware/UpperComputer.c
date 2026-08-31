/**
 ******************************************************************************
 * @file    UpperComputer.c
 * @brief   Original upper-computer packet receive and assignment logic
 * @pin_resources PA2=USART2_TX and PA3=USART2_RX through UART.c.
 * @peripherals USART2 receive interrupt.
 * @function Collects ten bytes, checks B3/B4, parses fields and assigns commands.
 * @purpose Feeds the original chassis command values into heading control.
 * @migration Packet bytes and chassis assignments match src/Hardwaer/usart_parse.c.
 ******************************************************************************
 */

#include "UpperComputer.h"

#include "UART.h"

uint8_t rx_data;
uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
uint8_t rx_cnt = 0U;
volatile uint8_t rx_complete_flag = 0U;
volatile uint8_t parsing_in_progress = 0U;
uint16_t motor_vx = 0U;
uint16_t motor_vy = 0U;
float target_yaw = 0.0f;

static UART_Packet_t rx_packet;

static uint8_t UART_Validate_Packet(const uint8_t *buffer, uint8_t length)
{
    if (length != UART_PACKET_LENGTH)
    {
        return 0U;
    }
    if (buffer[0] != UART_PACKET_HEADER)
    {
        return 0U;
    }
    if (buffer[UART_PACKET_LENGTH - 1U] != UART_PACKET_FOOTER)
    {
        return 0U;
    }
    return 1U;
}

void UART_Enable_Receive(void)
{
    rx_complete_flag = 0U;
    rx_cnt = 0U;
    (void)HAL_UART_Receive_IT(&huart2, &rx_data, 1U);
}

void UART_Disable_Receive(void)
{
    parsing_in_progress = 1U;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (parsing_in_progress != 0U)
        {
            (void)HAL_UART_Receive_IT(&huart2, &rx_data, 1U);
            return;
        }

        rx_buffer[rx_cnt++] = rx_data;
        if (rx_cnt >= UART_PACKET_LENGTH)
        {
            if (UART_Validate_Packet(rx_buffer, rx_cnt) != 0U)
            {
                rx_complete_flag = 1U;
            }
            else
            {
                rx_cnt = 0U;
            }
        }
        else if (rx_cnt >= sizeof(rx_buffer))
        {
            rx_cnt = 0U;
        }

        if (rx_complete_flag == 0U)
        {
            (void)HAL_UART_Receive_IT(&huart2, &rx_data, 1U);
        }
    }
}

void UART_Parse_Data(void)
{
    if (rx_complete_flag == 0U)
    {
        return;
    }

    parsing_in_progress = 1U;
    rx_packet.header = rx_buffer[0];
    rx_packet.forward_speed = rx_buffer[1];
    rx_packet.horizontal_speed = rx_buffer[2];
    rx_packet.target_angle = rx_buffer[3];
    rx_packet.rudder_angle = rx_buffer[4];
    rx_packet.lift_rod = rx_buffer[5];
    rx_packet.horizontal_rod = rx_buffer[6];
    rx_packet.switch_one = rx_buffer[7];
    rx_packet.switch_two = rx_buffer[8];
    rx_packet.footer = rx_buffer[9];
    parsing_in_progress = 0U;
    rx_cnt = 0U;
    rx_complete_flag = 0U;
    (void)HAL_UART_Receive_IT(&huart2, &rx_data, 1U);
}

void UART_Launch(void)
{
    motor_vx = rx_packet.forward_speed;
    motor_vy = rx_packet.horizontal_speed;
    target_yaw = rx_packet.target_angle;
}

const UART_Packet_t *UART_GetLatestPacket(void)
{
    return &rx_packet;
}
