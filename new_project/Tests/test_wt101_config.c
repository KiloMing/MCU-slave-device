/**
 ******************************************************************************
 * @file    test_wt101_config.c
 * @brief   Host regression test for the HWT101 UART configuration
 * @pin_resources PB6=USART1_TX, PB7=USART1_RX; no physical pins used on host.
 * @function Verifies that the photographed bare HWT101 module uses 115200 baud.
 * @purpose Prevents mixing the HWT101 module setting with HWT101CT defaults.
 ******************************************************************************
 */
#include <stdio.h>

#include "WT101.h"

int main(void)
{
    if (WT101_UART_BAUD_RATE != 115200U)
    {
        fprintf(stderr,
                "FAIL: HWT101 module baud is %lu, expected 115200\n",
                (unsigned long)WT101_UART_BAUD_RATE);
        return 1;
    }

    puts("PASS: HWT101 module UART baud is 115200");
    return 0;
}
