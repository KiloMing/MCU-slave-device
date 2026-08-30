/**
 ******************************************************************************
 * @file    main.c
 * @brief   Isolated HAL test entry forwarding HWT101 UART frames through USART2
 *
 * @pin_resources
 *   - UART: PA2 TX, PA3 RX, PA6 RS485 DE, PA7 RS485 /RE.
 *   - Motor PWM: PA0, PA1, PA8 and PA11.
 *   - Motor direction: PA4, PB3, PB4, PB5, PB12, PB13, PB14 and PB15.
 *   - HWT101 UART: PB6 USART1_TX and PB7 USART1_RX after USART1 remap.
 *   - PA13 : SWDIO, reserved for programming and debugging.
 *   - PA14 : SWCLK, reserved for programming and debugging.
 *
 * @peripherals
 *   - RCC, SysTick, USART1, USART2, TIM1, TIM2, GPIOA, GPIOB and AFIO.
 *
 * @function
 *   - UART mode: sends the verified ready text, then echoes received bytes.
 *   - Motor mode: PWM 500 forward 10 s, reverse 10 s, continuously.
 *   - WT101 mode: forwards 11-byte raw frames and checksum state through USART2.
 *
 * @purpose
 *   - Independently verifies HWT101 UART acquisition and TTL debug output.
 *
 * @migration
 *   - Sources: verified E:\project_M\test_p UART and motor tests.
 *   - HWT101 input and USART2 debug output: 115200 8N1 on separate UARTs.
 *   - Unchanged: PWM 500 and 10-second direction intervals.
 *   - APP_TEST_MODE currently selects the isolated WT101 test.
 ******************************************************************************
 */

#include "stm32f1xx_hal.h"
#include "App_Test.h"

static void SystemClock_Config(void);
static void Error_Handler(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

#if APP_TEST_MODE == APP_TEST_UART
    App_Test_UART_Init();

    while (1)
    {
        App_Test_UART_ProcessByte();
    }
#elif APP_TEST_MODE == APP_TEST_MOTOR
    App_Test_Motor_Init();

    while (1)
    {
        App_Test_Motor_RunCycle();
    }
#elif APP_TEST_MODE == APP_TEST_WT101
    App_Test_WT101_Init();
    while (1)
    {
        App_Test_WT101_RunStep();
    }
#else
#error "APP_TEST_MODE must be APP_TEST_UART, APP_TEST_MOTOR or APP_TEST_WT101"
#endif
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

static void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
