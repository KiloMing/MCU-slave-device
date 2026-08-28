#include "ultrasound.h"
#include "tim.h"

typedef enum {
    ULTRASOUND_IDLE = 0,
    ULTRASOUND_WAIT_RISING,
    ULTRASOUND_WAIT_FALLING,
    ULTRASOUND_CAPTURE_COMPLETE
} Ultrasound_State_t;

static volatile Ultrasound_State_t ultrasound_state = ULTRASOUND_IDLE;
static volatile uint16_t echo_rising_count;
static volatile uint16_t echo_pulse_us;
static uint32_t trigger_tick;
static uint32_t last_trigger_tick;
static uint16_t result_distance_mm;
static bool result_valid;
static bool result_ready;

static void Ultrasound_Trigger(void)
{
    uint16_t pulse_start;

    __HAL_TIM_SET_CAPTUREPOLARITY(&htim4,
                                  TIM_CHANNEL_2,
                                  TIM_INPUTCHANNELPOLARITY_RISING);
    __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_CC2);
    ultrasound_state = ULTRASOUND_WAIT_RISING;
    trigger_tick = HAL_GetTick();
    last_trigger_tick = trigger_tick;

    HAL_GPIO_WritePin(ULTRASOUND_TRIG_GPIO_Port,
                      ULTRASOUND_TRIG_Pin,
                      GPIO_PIN_SET);

    pulse_start = (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);
    while ((uint16_t)((uint16_t)__HAL_TIM_GET_COUNTER(&htim4) - pulse_start) < 12U) {
        /* HC-SR04 requires a trigger pulse of at least 10 us. */
    }

    HAL_GPIO_WritePin(ULTRASOUND_TRIG_GPIO_Port,
                      ULTRASOUND_TRIG_Pin,
                      GPIO_PIN_RESET);
}

void Ultrasound_Init(void)
{
    ultrasound_state = ULTRASOUND_IDLE;
    echo_rising_count = 0U;
    echo_pulse_us = 0U;
    trigger_tick = 0U;
    last_trigger_tick = 0U;
    result_distance_mm = 0U;
    result_valid = false;
    result_ready = false;

    HAL_GPIO_WritePin(ULTRASOUND_TRIG_GPIO_Port,
                      ULTRASOUND_TRIG_Pin,
                      GPIO_PIN_RESET);
    __HAL_TIM_SET_COUNTER(&htim4, 0U);
    __HAL_TIM_SET_CAPTUREPOLARITY(&htim4,
                                  TIM_CHANNEL_2,
                                  TIM_INPUTCHANNELPOLARITY_RISING);
    HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_2);
}

void Ultrasound_Update(void)
{
    uint32_t now = HAL_GetTick();

    if (ultrasound_state == ULTRASOUND_CAPTURE_COMPLETE) {
        uint32_t distance_mm = ((uint32_t)echo_pulse_us * 10U + 29U) / 58U;

        if ((distance_mm >= ULTRASOUND_MIN_DISTANCE_MM) &&
            (distance_mm <= ULTRASOUND_MAX_DISTANCE_MM)) {
            result_distance_mm = (uint16_t)distance_mm;
            result_valid = true;
        } else {
            result_distance_mm = 0U;
            result_valid = false;
        }

        result_ready = true;
        ultrasound_state = ULTRASOUND_IDLE;
    } else if ((ultrasound_state == ULTRASOUND_WAIT_RISING) ||
               (ultrasound_state == ULTRASOUND_WAIT_FALLING)) {
        if ((now - trigger_tick) >= ULTRASOUND_ECHO_TIMEOUT_MS) {
            __HAL_TIM_SET_CAPTUREPOLARITY(&htim4,
                                          TIM_CHANNEL_2,
                                          TIM_INPUTCHANNELPOLARITY_RISING);
            result_distance_mm = 0U;
            result_valid = false;
            result_ready = true;
            ultrasound_state = ULTRASOUND_IDLE;
        }
    }

    if ((ultrasound_state == ULTRASOUND_IDLE) &&
        ((now - last_trigger_tick) >= ULTRASOUND_TRIGGER_INTERVAL_MS)) {
        Ultrasound_Trigger();
    }
}

bool Ultrasound_TakeMeasurement(uint16_t *distance_mm, bool *valid)
{
    if (!result_ready) {
        return false;
    }

    if (distance_mm != NULL) {
        *distance_mm = result_distance_mm;
    }
    if (valid != NULL) {
        *valid = result_valid;
    }

    result_ready = false;
    return true;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if ((htim->Instance != TIM4) ||
        (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_2)) {
        return;
    }

    if (ultrasound_state == ULTRASOUND_WAIT_RISING) {
        echo_rising_count =
            (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        __HAL_TIM_SET_CAPTUREPOLARITY(htim,
                                      TIM_CHANNEL_2,
                                      TIM_INPUTCHANNELPOLARITY_FALLING);
        ultrasound_state = ULTRASOUND_WAIT_FALLING;
    } else if (ultrasound_state == ULTRASOUND_WAIT_FALLING) {
        uint16_t echo_falling_count =
            (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

        echo_pulse_us = (uint16_t)(echo_falling_count - echo_rising_count);
        __HAL_TIM_SET_CAPTUREPOLARITY(htim,
                                      TIM_CHANNEL_2,
                                      TIM_INPUTCHANNELPOLARITY_RISING);
        ultrasound_state = ULTRASOUND_CAPTURE_COMPLETE;
    }
}
