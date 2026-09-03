/**
 ******************************************************************************
 * @file    MotorEncoder.c
 * @brief   Four-channel chassis motor encoder acquisition
 * @pin_resources
 *   - Motor A/LF: PB6 TIM4_CH1 and PB7 TIM4_CH2.
 *   - Motor B/LB: PA6 TIM3_CH1 and PA7 TIM3_CH2.
 *   - Motor C/RF: PB0/PB1 GPIO EXTI rising and falling edges.
 *   - Motor D/RB: PA5/PA12 GPIO EXTI rising and falling edges.
 * @peripherals TIM3 encoder mode, TIM4 encoder mode, GPIOA/B and EXTI.
 * @function Uses hardware quadrature for A/B and software quadrature for C/D.
 * @purpose Produces four signed 10 ms pulse samples for four independent PIDs.
 * @migration Preserves speed=pulse*60000*2/(11*15*10) from the old project.
 ******************************************************************************
 */

#include "MotorEncoder.h"
#include "stm32f1xx_hal.h"

static TIM_HandleTypeDef motor_encoder_tim3;
static TIM_HandleTypeDef motor_encoder_tim4;
static volatile int32_t motor_encoder_soft_count_c;
static volatile int32_t motor_encoder_soft_count_d;
static volatile uint8_t motor_encoder_state_c;
static volatile uint8_t motor_encoder_state_d;

static void MotorEncoder_FailStop(void)
{
    while (1)
    {
    }
}

static uint8_t MotorEncoder_ReadState(GPIO_TypeDef *port,
                                      uint16_t phase_a_pin,
                                      uint16_t phase_b_pin)
{
    uint8_t phase_a = (HAL_GPIO_ReadPin(port, phase_a_pin) == GPIO_PIN_SET) ?
                      1U : 0U;
    uint8_t phase_b = (HAL_GPIO_ReadPin(port, phase_b_pin) == GPIO_PIN_SET) ?
                      1U : 0U;
    return (uint8_t)(phase_a | (uint8_t)(phase_b << 1U));
}

static void MotorEncoder_ConfigHardwareTimer(TIM_HandleTypeDef *timer,
                                              void *instance)
{
    TIM_Encoder_InitTypeDef encoder = {0};

    timer->Instance = instance;
    timer->Init.Prescaler = 0U;
    timer->Init.CounterMode = TIM_COUNTERMODE_UP;
    timer->Init.Period = 65535U;
    timer->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    encoder.EncoderMode = TIM_ENCODERMODE_TI12;
    encoder.IC1Polarity = TIM_ICPOLARITY_RISING;
    encoder.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    encoder.IC1Prescaler = TIM_ICPSC_DIV1;
    encoder.IC1Filter = 0U;
    encoder.IC2Polarity = TIM_ICPOLARITY_RISING;
    encoder.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    encoder.IC2Prescaler = TIM_ICPSC_DIV1;
    encoder.IC2Filter = 0U;

    if (HAL_TIM_Encoder_Init(timer, &encoder) != HAL_OK)
    {
        MotorEncoder_FailStop();
    }
    __HAL_TIM_SET_COUNTER(timer, 0U);
    if (HAL_TIM_Encoder_Start(timer, TIM_CHANNEL_ALL) != HAL_OK)
    {
        MotorEncoder_FailStop();
    }
}

void MotorEncoder_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &gpio);
    HAL_GPIO_Init(GPIOB, &gpio);

    MotorEncoder_ConfigHardwareTimer(&motor_encoder_tim4, TIM4);
    MotorEncoder_ConfigHardwareTimer(&motor_encoder_tim3, TIM3);

    gpio.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_12;
    HAL_GPIO_Init(GPIOA, &gpio);

    motor_encoder_soft_count_c = 0;
    motor_encoder_soft_count_d = 0;
    motor_encoder_state_c = MotorEncoder_ReadState(GPIOB, GPIO_PIN_0,
                                                    GPIO_PIN_1);
    motor_encoder_state_d = MotorEncoder_ReadState(GPIOA, GPIO_PIN_5,
                                                    GPIO_PIN_12);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 2U, 0U);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    HAL_NVIC_SetPriority(EXTI1_IRQn, 2U, 0U);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2U, 0U);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2U, 0U);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void MotorEncoder_EXTI_Callback(uint16_t gpio_pin)
{
    if ((gpio_pin == GPIO_PIN_0) || (gpio_pin == GPIO_PIN_1))
    {
        uint8_t state = MotorEncoder_ReadState(GPIOB, GPIO_PIN_0, GPIO_PIN_1);
        motor_encoder_soft_count_c +=
            MotorEncoder_DecodeStep(motor_encoder_state_c, state);
        motor_encoder_state_c = state;
    }
    else if ((gpio_pin == GPIO_PIN_5) || (gpio_pin == GPIO_PIN_12))
    {
        uint8_t state = MotorEncoder_ReadState(GPIOA, GPIO_PIN_5, GPIO_PIN_12);
        motor_encoder_soft_count_d +=
            MotorEncoder_DecodeStep(motor_encoder_state_d, state);
        motor_encoder_state_d = state;
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin)
{
    MotorEncoder_EXTI_Callback(gpio_pin);
}

void MotorEncoder_Sample(int32_t pulses[MOTOR_ENCODER_COUNT],
                         int32_t speeds[MOTOR_ENCODER_COUNT])
{
    uint32_t interrupt_state;
    uint32_t index;

    if ((pulses == NULL) || (speeds == NULL))
    {
        return;
    }

    pulses[0] = (int32_t)(int16_t)__HAL_TIM_GET_COUNTER(&motor_encoder_tim4);
    pulses[1] = (int32_t)(int16_t)__HAL_TIM_GET_COUNTER(&motor_encoder_tim3);
    __HAL_TIM_SET_COUNTER(&motor_encoder_tim4, 0U);
    __HAL_TIM_SET_COUNTER(&motor_encoder_tim3, 0U);

    interrupt_state = __get_PRIMASK();
    __disable_irq();
    pulses[2] = motor_encoder_soft_count_c;
    pulses[3] = motor_encoder_soft_count_d;
    motor_encoder_soft_count_c = 0;
    motor_encoder_soft_count_d = 0;
    if (interrupt_state == 0U)
    {
        __enable_irq();
    }

    for (index = 0U; index < MOTOR_ENCODER_COUNT; index++)
    {
        speeds[index] = MotorEncoder_PulseToSpeed(pulses[index]);
    }
}
