#ifndef ULTRASOUND_H
#define ULTRASOUND_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

#define ULTRASOUND_MIN_DISTANCE_MM       20U
#define ULTRASOUND_MAX_DISTANCE_MM       4000U
#define ULTRASOUND_TRIGGER_INTERVAL_MS   60U
#define ULTRASOUND_ECHO_TIMEOUT_MS       35U

void Ultrasound_Init(void);
void Ultrasound_Update(void);
bool Ultrasound_TakeMeasurement(uint16_t *distance_mm, bool *valid);

#endif /* ULTRASOUND_H */
