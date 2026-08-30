/**
 ******************************************************************************
 * @file    App_Test.h
 * @brief   Selectable UART and motor migration-test interface
 *
 * @pin_resources
 *   - UART: PA2, PA3, PA6 and PA7.
 *   - Motor PWM: PA0, PA1, PA8 and PA11.
 *   - Motor direction: PA4, PB3, PB4, PB5, PB12, PB13, PB14 and PB15.
 *
 * @peripherals
 *   - USART2, TIM1, TIM2, GPIO and SysTick.
 *
 * @function
 *   - Provides finite initialization and processing steps for each test mode.
 *
 * @purpose
 *   - Keeps UART and motor tests isolated while making their behavior testable.
 *
 * @migration
 *   - Sources: verified test_p UART and four-motor test loops.
 *   - Selected: four-motor mode is the current power-on test.
 *   - Unchanged: UART ready text, echo, motor speeds and timing.
 ******************************************************************************
 */

#ifndef APP_TEST_H
#define APP_TEST_H

#define APP_TEST_UART  1U
#define APP_TEST_MOTOR 2U

#ifndef APP_TEST_MODE
#define APP_TEST_MODE APP_TEST_MOTOR
#endif

void App_Test_UART_Init(void);
void App_Test_UART_ProcessByte(void);
void App_Test_Motor_Init(void);
void App_Test_Motor_RunCycle(void);

#endif
