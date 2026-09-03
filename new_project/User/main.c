/**
 ******************************************************************************
 * @file    main.c
 * @brief   Bluetooth four-wheel encoder PID chassis test entry
 * @pin_resources
 *   - HC-08 UART: PA2=USART2_TX, PA3=USART2_RX, common GND.
 *   - Activity LED: PC13 through 510 ohm to an external LED.
 *   - Motor PWM: PA0, PA1, PA8 and PA11.
 *   - Motor direction: PA4, PB3/PB4/PB5/PB12/PB13/PB14/PB15.
 *   - Motor A encoder: PB6/PB7; Motor B encoder: PA6/PA7.
 *   - Motor C encoder: A phase PB1, B phase PB0.
 *   - Motor D encoder: A phase PA12, B phase PA5.
 *   - PA13=SWDIO and PA14=SWCLK remain reserved for debugging.
 * @peripherals USART2, TIM1-TIM4, GPIOA/B/C, AFIO, EXTI and SysTick.
 * @function Receives [j,LX,LY,RX,RY] and runs four independent speed PIDs.
 * @purpose Tests closed-loop Mecanum translation and rotation over Bluetooth.
 * @migration Keeps the packet axes, wheel equations, encoder conversion and
 *            tuned per-motor PID gains established by the staged tests.
 ******************************************************************************
 */

#include "stm32f1xx_hal.h"
#include "buleteethtest.h"

static void SystemClock_Config(void);
static void Error_Handler(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    BluetoothTest_Init();

    while (1)
    {
        BluetoothTest_RunStep();
    }
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
