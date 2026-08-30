/**
 ******************************************************************************
 * @file    App_Test.h
 * @brief   Selectable UART, motor and WT101 migration-test interface
 *
 * @pin_resources
 *   - UART: PA2, PA3, PA6 and PA7.
 *   - Motor PWM: PA0, PA1, PA8 and PA11.
 *   - Motor direction: PA4, PB3, PB4, PB5, PB12, PB13, PB14 and PB15.
 *   - HWT101 UART: PB6=USART1_TX and PB7=USART1_RX.
 *
 * @peripherals
 *   - Remapped USART1, USART2, TIM1, TIM2, GPIO and SysTick.
 *
 * @function
 *   - Provides finite initialization and processing steps for each test mode.
 *
 * @purpose
 *   - Keeps UART and motor tests isolated while making their behavior testable.
 *
 * @migration
 *   - Sources: verified test_p UART and four-motor test loops.
 *   - Selected: HWT101 raw UART frames are the current isolated power-on test.
 *   - Unchanged: motor speeds, motor timing and USART2 debug interface.
 *   - HWT101 input and debug output both use 115200 8N1 on separate UARTs.
 *   - Current output: hexadecimal 11-byte frames with checksum status.
 ******************************************************************************
 */

#ifndef APP_TEST_H
#define APP_TEST_H

#define APP_TEST_UART  1U
#define APP_TEST_MOTOR 2U
#define APP_TEST_WT101 3U

#ifndef APP_TEST_MODE
#define APP_TEST_MODE APP_TEST_WT101
#endif

void App_Test_UART_Init(void);
void App_Test_UART_ProcessByte(void);
void App_Test_Motor_Init(void);
void App_Test_Motor_RunCycle(void);
void App_Test_WT101_Init(void);
void App_Test_WT101_RunStep(void);

#endif
