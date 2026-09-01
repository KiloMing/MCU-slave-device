/**
 ******************************************************************************
 * @file    stm32f1xx_hal.h
 * @brief   Host-test HAL boundary for UART and motor migration tests
 *
 * @pin_resources
 *   - Models GPIOA/GPIOB motor/UART pins and the PC13 activity LED.
 *
 * @peripherals
 *   - USART2, TIM1, TIM2, GPIO and RCC test doubles.
 *
 * @function
 *   - Supplies only the HAL types and calls required by the real drivers.
 *
 * @purpose
 *   - Executes driver behavior on the host without connecting physical hardware.
 ******************************************************************************
 */

#ifndef TEST_STM32F1XX_HAL_H
#define TEST_STM32F1XX_HAL_H

#include <stddef.h>
#include <stdint.h>

typedef enum
{
    HAL_OK = 0,
    HAL_ERROR = 1,
    HAL_BUSY = 2,
    HAL_TIMEOUT = 3
} HAL_StatusTypeDef;

typedef struct
{
    uint32_t id;
} GPIO_TypeDef;

typedef struct
{
    uint16_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
} GPIO_InitTypeDef;

typedef enum
{
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET
} GPIO_PinState;

typedef struct
{
    uint32_t BaudRate;
    uint32_t WordLength;
    uint32_t StopBits;
    uint32_t Parity;
    uint32_t Mode;
    uint32_t HwFlowCtl;
    uint32_t OverSampling;
} UART_InitTypeDef;

typedef struct
{
    void *Instance;
    UART_InitTypeDef Init;
} UART_HandleTypeDef;

typedef struct
{
    uint32_t Prescaler;
    uint32_t CounterMode;
    uint32_t Period;
    uint32_t ClockDivision;
    uint32_t RepetitionCounter;
    uint32_t AutoReloadPreload;
} TIM_Base_InitTypeDef;

typedef struct
{
    void *Instance;
    TIM_Base_InitTypeDef Init;
} TIM_HandleTypeDef;

typedef struct
{
    uint32_t OCMode;
    uint32_t Pulse;
    uint32_t OCPolarity;
    uint32_t OCNPolarity;
    uint32_t OCFastMode;
    uint32_t OCIdleState;
    uint32_t OCNIdleState;
} TIM_OC_InitTypeDef;

typedef struct
{
    uint32_t MasterOutputTrigger;
    uint32_t MasterSlaveMode;
} TIM_MasterConfigTypeDef;

extern GPIO_TypeDef test_gpioa;
extern GPIO_TypeDef test_gpiob;
extern GPIO_TypeDef test_gpioc;
extern uint32_t test_tim1_instance;
extern uint32_t test_tim2_instance;
extern uint32_t test_usart2_instance;
extern uint32_t test_usart1_instance;

#define GPIOA (&test_gpioa)
#define GPIOB (&test_gpiob)
#define GPIOC (&test_gpioc)
#define TIM1 ((void *)&test_tim1_instance)
#define TIM2 ((void *)&test_tim2_instance)
#define USART2 ((void *)&test_usart2_instance)
#define USART1 ((void *)&test_usart1_instance)
#define USART2_IRQn 38

#define GPIO_PIN_0  ((uint16_t)0x0001U)
#define GPIO_PIN_1  ((uint16_t)0x0002U)
#define GPIO_PIN_2  ((uint16_t)0x0004U)
#define GPIO_PIN_3  ((uint16_t)0x0008U)
#define GPIO_PIN_4  ((uint16_t)0x0010U)
#define GPIO_PIN_5  ((uint16_t)0x0020U)
#define GPIO_PIN_6  ((uint16_t)0x0040U)
#define GPIO_PIN_7  ((uint16_t)0x0080U)
#define GPIO_PIN_8  ((uint16_t)0x0100U)
#define GPIO_PIN_11 ((uint16_t)0x0800U)
#define GPIO_PIN_12 ((uint16_t)0x1000U)
#define GPIO_PIN_13 ((uint16_t)0x2000U)
#define GPIO_PIN_14 ((uint16_t)0x4000U)
#define GPIO_PIN_15 ((uint16_t)0x8000U)

#define GPIO_MODE_OUTPUT_PP 0x01U
#define GPIO_MODE_AF_PP     0x02U
#define GPIO_MODE_INPUT     0x03U
#define GPIO_NOPULL         0x00U
#define GPIO_SPEED_FREQ_HIGH 0x03U

#define UART_WORDLENGTH_8B 0x00U
#define UART_STOPBITS_1 0x00U
#define UART_PARITY_NONE 0x00U
#define UART_MODE_TX_RX 0x03U
#define UART_HWCONTROL_NONE 0x00U
#define UART_OVERSAMPLING_16 0x00U

#define TIM_COUNTERMODE_UP 0x00U
#define TIM_CLOCKDIVISION_DIV1 0x00U
#define TIM_AUTORELOAD_PRELOAD_DISABLE 0x00U
#define TIM_OCMODE_PWM1 0x01U
#define TIM_OCPOLARITY_HIGH 0x00U
#define TIM_OCNPOLARITY_HIGH 0x00U
#define TIM_OCFAST_DISABLE 0x00U
#define TIM_OCIDLESTATE_RESET 0x00U
#define TIM_OCNIDLESTATE_RESET 0x00U
#define TIM_TRGO_RESET 0x00U
#define TIM_MASTERSLAVEMODE_DISABLE 0x00U
#define TIM_CHANNEL_1 1U
#define TIM_CHANNEL_2 2U
#define TIM_CHANNEL_4 4U
#define HAL_MAX_DELAY 0xFFFFFFFFU

#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOC_CLK_ENABLE() ((void)0)
#define __HAL_RCC_AFIO_CLK_ENABLE() ((void)0)
#define __HAL_RCC_USART2_CLK_ENABLE() ((void)0)
#define __HAL_RCC_USART1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_TIM1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_TIM2_CLK_ENABLE() ((void)0)
void Test_HAL_SwjNoJtag(void);
#define __HAL_AFIO_REMAP_SWJ_NOJTAG() Test_HAL_SwjNoJtag()
#define __HAL_AFIO_REMAP_USART1_ENABLE() ((void)0)

void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *data,
                                   uint16_t length, uint32_t timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *data,
                                  uint16_t length, uint32_t timeout);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *data,
                                     uint16_t length);
void HAL_UART_IRQHandler(UART_HandleTypeDef *huart);
void HAL_NVIC_SetPriority(int irq, uint32_t priority, uint32_t subpriority);
void HAL_NVIC_EnableIRQ(int irq);
uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIMEx_MasterConfigSynchronization(TIM_HandleTypeDef *htim,
                                                        TIM_MasterConfigTypeDef *config);
HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef *htim,
                                            TIM_OC_InitTypeDef *config,
                                            uint32_t channel);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t channel);
void Test_HAL_SetCompare(TIM_HandleTypeDef *htim, uint32_t channel, uint32_t value);
void HAL_Delay(uint32_t delay_ms);

#define __HAL_TIM_SET_COMPARE(htim, channel, value) \
    Test_HAL_SetCompare((htim), (channel), (value))

#endif
