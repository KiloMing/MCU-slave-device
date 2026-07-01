#include "water_pump.h"

static bool water_pump_is_on;

void WaterPump_Init(void)
{
    water_pump_is_on = false;
    HAL_GPIO_WritePin(WATER_PUMP_GPIO_Port, WATER_PUMP_Pin, GPIO_PIN_RESET);
}

void WaterPump_Set(bool enabled)
{
    HAL_GPIO_WritePin(WATER_PUMP_GPIO_Port,
                      WATER_PUMP_Pin,
                      enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
    water_pump_is_on = enabled;
}

void WaterPump_On(void)
{
    WaterPump_Set(true);
}

void WaterPump_Off(void)
{
    WaterPump_Set(false);
}

bool WaterPump_IsOn(void)
{
    return water_pump_is_on;
}
