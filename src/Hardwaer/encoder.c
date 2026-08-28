#include "encoder.h"

int32_t encoder_left_count = 0;
int32_t encoder_right_count = 0;
int32_t encoder_left_speed = 0;
int32_t encoder_right_speed = 0;

// encoder_calculate_speed() is called by the 10 ms control timer.
#define ENCODER_SAMPLE_PERIOD_MS 10
#define ENCODER_RESOLUTION 11
#define GEAR_RATIO 15
#define SPEED_SCALE 2

static int16_t encoder_sample_left_pulse(void)
{
    int16_t pulse_count = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    return pulse_count;
}

static int16_t encoder_sample_right_pulse(void)
{
    int16_t pulse_count = (int16_t)__HAL_TIM_GET_COUNTER(&htim4);
    __HAL_TIM_SET_COUNTER(&htim4, 0);
    return pulse_count;
}

void encoder_init(void)
{
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_1 | TIM_CHANNEL_2);
    HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_1 | TIM_CHANNEL_2);

    encoder_reset_left();
    encoder_reset_right();
}

int32_t encoder_get_left_count(void)
{
    return (int32_t)((int16_t)__HAL_TIM_GET_COUNTER(&htim3));
}

int32_t encoder_get_right_count(void)
{
    return (int32_t)((int16_t)__HAL_TIM_GET_COUNTER(&htim4));
}

void encoder_reset_left(void)
{
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    encoder_left_count = 0;
    encoder_left_speed = 0;
}

void encoder_reset_right(void)
{
    __HAL_TIM_SET_COUNTER(&htim4, 0);
    encoder_right_count = 0;
    encoder_right_speed = 0;
}

/**
 * @brief 计算电机速度
 * @retval 无
 * @note 该函数固定由 10 ms 定时器调用一次。
 *       speed = (本周期脉冲数 * 60000 * 缩放系数) /
 *               (编码器分辨率 * 减速比 * 采样周期)
 */
void encoder_calculate_speed(void)
{
    int16_t left_pulse = encoder_sample_left_pulse();
    int16_t right_pulse = encoder_sample_right_pulse();

    encoder_left_count = (int32_t)left_pulse;
    encoder_right_count = (int32_t)right_pulse;

    encoder_left_speed = (int32_t)((left_pulse * 60000LL * SPEED_SCALE) /
                                   (ENCODER_RESOLUTION * GEAR_RATIO * ENCODER_SAMPLE_PERIOD_MS));
    encoder_right_speed = (int32_t)((right_pulse * 60000LL * SPEED_SCALE) /
                                    (ENCODER_RESOLUTION * GEAR_RATIO * ENCODER_SAMPLE_PERIOD_MS));
}
