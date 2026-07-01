#ifndef MILK_MONITOR_H
#define MILK_MONITOR_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Set both values after measuring the installed tank:
 * - FULL: sensor-to-milk distance when the tank is full
 * - EMPTY: sensor-to-bottom distance when the tank is empty
 * Leaving either value at zero keeps percentage reporting uncalibrated (0xFF).
 */
#define MILK_FULL_DISTANCE_MM       0U
#define MILK_EMPTY_DISTANCE_MM      0U

#define MILK_TELEMETRY_INTERVAL_MS  200U
#define MILK_MEASUREMENT_STALE_MS   250U

#define MILK_TELEMETRY_HEADER       0xB5U
#define MILK_TELEMETRY_VERSION      0x01U
#define MILK_TELEMETRY_FOOTER       0xB6U
#define MILK_TELEMETRY_LENGTH       10U

#define MILK_STATUS_DISTANCE_VALID  (1U << 0)
#define MILK_STATUS_CALIBRATED      (1U << 1)
#define MILK_STATUS_PUMP_REQUESTED  (1U << 2)
#define MILK_STATUS_FORCED_OFF      (1U << 3)

void MilkMonitor_Init(void);
void MilkMonitor_Update(void);
void MilkMonitor_SetPumpRequest(bool requested);
bool MilkMonitor_GetPumpRequest(void);

#endif /* MILK_MONITOR_H */
