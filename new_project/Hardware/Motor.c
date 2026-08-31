/**
 ******************************************************************************
 * @file    Motor.c
 * @brief   Four-channel D24A open-loop motor driver
 *
 * @pin_resources
 *   - LF: PA0 TIM2_CH1 PWM; PB5=direction high side, PB12=direction low side.
 *   - LB: PA1 TIM2_CH2 PWM; PB13/PB14 direction.
 *   - RF: PA8 TIM1_CH1 PWM; PB15/PA4 direction.
 *   - RB: PA11 TIM1_CH4 PWM; PB3/PB4 direction.
 *   - PB3/PB4 require JTAG disabled while SWD remains enabled.
 *
 * @peripherals
 *   - TIM1 CH1/CH4, TIM2 CH1/CH2, GPIOA, GPIOB and AFIO.
 *
 * @function
 *   - Produces four 0..999 PWM values, signed direction control and original
 *     Mecanum-wheel kinematic/PID distribution.
 *
 * @purpose
 *   - Reproduces the completed four-motor open-loop chassis test on HAL.
 *
 * @migration
 *   - Sources: E:\project_M\src\Hardwaer\motor.c and completed test_p test.
 *   - Unchanged: prescaler 6, period 999, signed direction and PWM magnitude.
 *   - Adapted: RF/RB PWM moved to TIM1 and final A/B/C wiring swaps retained.
 ******************************************************************************
 */

#include "Motor.h"
#include "Mulun.h"

#define MOTOR_PWM_PERIOD 999U
#define MOTOR_PWM_PRESCALER 6U

#define LF_DIR_HIGH_PORT GPIOB
#define LF_DIR_HIGH_PIN  GPIO_PIN_5
#define LF_DIR_LOW_PORT  GPIOB
#define LF_DIR_LOW_PIN   GPIO_PIN_12

#define LB_DIR_HIGH_PORT GPIOB
#define LB_DIR_HIGH_PIN  GPIO_PIN_13
#define LB_DIR_LOW_PORT  GPIOB
#define LB_DIR_LOW_PIN   GPIO_PIN_14

#define RF_DIR_HIGH_PORT GPIOB
#define RF_DIR_HIGH_PIN  GPIO_PIN_15
#define RF_DIR_LOW_PORT  GPIOA
#define RF_DIR_LOW_PIN   GPIO_PIN_4

#define RB_DIR_HIGH_PORT GPIOB
#define RB_DIR_HIGH_PIN  GPIO_PIN_3
#define RB_DIR_LOW_PORT  GPIOB
#define RB_DIR_LOW_PIN   GPIO_PIN_4

static PID_Mulun_HandleTypeDef mulun_pid;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

static void Motor_FailStop(void)
{
    while (1)
    {
    }
}

static uint32_t Motor_ClampPwm(int32_t speed)
{
    uint32_t pwm = (speed < 0) ? (uint32_t)(-speed) : (uint32_t)speed;
    return (pwm > MOTOR_PWM_PERIOD) ? MOTOR_PWM_PERIOD : pwm;
}

static void Motor_SetDirection(GPIO_TypeDef *high_port, uint16_t high_pin,
                               GPIO_TypeDef *low_port, uint16_t low_pin,
                               int32_t speed)
{
    if (speed >= 0)
    {
        HAL_GPIO_WritePin(high_port, high_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(low_port, low_pin, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(high_port, high_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(low_port, low_pin, GPIO_PIN_SET);
    }
}

static void Motor_ConfigTimer(TIM_HandleTypeDef *htim, void *instance)
{
    TIM_MasterConfigTypeDef master = {0};
    TIM_OC_InitTypeDef channel = {0};

    htim->Instance = instance;
    htim->Init.Prescaler = MOTOR_PWM_PRESCALER;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = MOTOR_PWM_PERIOD;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.RepetitionCounter = 0U;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(htim) != HAL_OK)
    {
        Motor_FailStop();
    }

    master.MasterOutputTrigger = TIM_TRGO_RESET;
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(htim, &master) != HAL_OK)
    {
        Motor_FailStop();
    }

    channel.OCMode = TIM_OCMODE_PWM1;
    channel.Pulse = 0U;
    channel.OCPolarity = TIM_OCPOLARITY_HIGH;
    channel.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    channel.OCFastMode = TIM_OCFAST_DISABLE;
    channel.OCIdleState = TIM_OCIDLESTATE_RESET;
    channel.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    if (HAL_TIM_PWM_ConfigChannel(htim, &channel, TIM_CHANNEL_1) != HAL_OK)
    {
        Motor_FailStop();
    }
    if (HAL_TIM_PWM_ConfigChannel(htim, &channel,
                                  (instance == TIM1) ? TIM_CHANNEL_4 : TIM_CHANNEL_2) != HAL_OK)
    {
        Motor_FailStop();
    }
}

static void Motor_ConfigGpio(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 |
                            GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
                      GPIO_PIN_RESET);

    gpio.Pin = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 |
               GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void Motor_Init(void)
{
    PID_Mulun_Init(&mulun_pid);
    Motor_ConfigGpio();
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    Motor_ConfigTimer(&htim1, TIM1);
    Motor_ConfigTimer(&htim2, TIM2);
    motor_PWM_Init();
    motor_stop_all();
}

void motor_PWM_Init(void)
{
    if ((HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1) != HAL_OK) ||
        (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2) != HAL_OK) ||
        (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) ||
        (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4) != HAL_OK))
    {
        Motor_FailStop();
    }
}

void motor_stop_all(void)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0U);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0U);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0U);
}

void Motor_LF_SetSpeed(int32_t speed)
{
    Motor_SetDirection(LF_DIR_HIGH_PORT, LF_DIR_HIGH_PIN,
                       LF_DIR_LOW_PORT, LF_DIR_LOW_PIN, speed);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, Motor_ClampPwm(speed));
}

void Motor_LB_SetSpeed(int32_t speed)
{
    Motor_SetDirection(LB_DIR_HIGH_PORT, LB_DIR_HIGH_PIN,
                       LB_DIR_LOW_PORT, LB_DIR_LOW_PIN, speed);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, Motor_ClampPwm(speed));
}

void Motor_RF_SetSpeed(int32_t speed)
{
    Motor_SetDirection(RF_DIR_HIGH_PORT, RF_DIR_HIGH_PIN,
                       RF_DIR_LOW_PORT, RF_DIR_LOW_PIN, speed);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, Motor_ClampPwm(speed));
}

void Motor_RB_SetSpeed(int32_t speed)
{
    Motor_SetDirection(RB_DIR_HIGH_PORT, RB_DIR_HIGH_PIN,
                       RB_DIR_LOW_PORT, RB_DIR_LOW_PIN, speed);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, Motor_ClampPwm(speed));
}

void motor_all_set(int32_t speed)
{
    if (speed == 0)
    {
        motor_stop_all();
        return;
    }

    Motor_LF_SetSpeed(speed);
    Motor_LB_SetSpeed(speed);
    Motor_RF_SetSpeed(speed);
    Motor_RB_SetSpeed(speed);
}

void mecanum_move(int32_t vx, int32_t vy, float omega)
{
    int32_t speed_lf = vx - vy + omega;
    int32_t speed_rf = vx + vy - omega;
    int32_t speed_lb = vx + vy + omega;
    int32_t speed_rb = vx - vy - omega;

    Motor_LF_SetSpeed(speed_lf);
    Motor_RF_SetSpeed(speed_rf);
    Motor_LB_SetSpeed(speed_lb);
    Motor_RB_SetSpeed(speed_rb);
}

void mecanum_with_heading_control(uint16_t vx, uint16_t vy,
                                  float requested_yaw, float current_yaw)
{
    float pid_data = PID_Mulun_Calc(&mulun_pid, requested_yaw, current_yaw);
    mecanum_move(vx, vy, pid_data);
}
