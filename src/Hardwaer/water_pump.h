#ifndef WATER_PUMP_H
#define WATER_PUMP_H

#include <stdbool.h>
#include "main.h"

void WaterPump_Init(void);
void WaterPump_Set(bool enabled);
void WaterPump_On(void);
void WaterPump_Off(void);
bool WaterPump_IsOn(void);

#endif /* WATER_PUMP_H */
