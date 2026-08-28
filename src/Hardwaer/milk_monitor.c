#include "milk_monitor.h"
#include "ultrasound.h"
#include "water_pump.h"
#include "usart.h"

static bool pump_requested;
static bool distance_valid;
static bool has_measurement;
static uint16_t distance_mm;
static uint32_t last_measurement_tick;
static uint32_t last_telemetry_tick;
static uint8_t telemetry_sequence;

static bool MilkMonitor_IsCalibrated(void)
{
    return (MILK_FULL_DISTANCE_MM > 0U) &&
           (MILK_EMPTY_DISTANCE_MM > MILK_FULL_DISTANCE_MM);
}

static uint8_t MilkMonitor_GetLevelPercent(bool measurement_safe)
{
#if (MILK_FULL_DISTANCE_MM > 0U) && \
    (MILK_EMPTY_DISTANCE_MM > MILK_FULL_DISTANCE_MM)
    uint32_t level;

    if (!measurement_safe) {
        return 0xFFU;
    }
    if (distance_mm <= MILK_FULL_DISTANCE_MM) {
        return 100U;
    }
    if (distance_mm >= MILK_EMPTY_DISTANCE_MM) {
        return 0U;
    }

    level = ((uint32_t)(MILK_EMPTY_DISTANCE_MM - distance_mm) * 100U) /
            (uint32_t)(MILK_EMPTY_DISTANCE_MM - MILK_FULL_DISTANCE_MM);
    return (uint8_t)level;
#else
    (void)measurement_safe;
    return 0xFFU;
#endif
}

static bool MilkMonitor_IsMeasurementSafe(uint32_t now)
{
    return has_measurement &&
           distance_valid &&
           ((now - last_measurement_tick) <= MILK_MEASUREMENT_STALE_MS);
}

static void MilkMonitor_SendTelemetry(uint32_t now)
{
    uint8_t packet[MILK_TELEMETRY_LENGTH] = {0U};
    uint8_t status = 0U;
    bool measurement_safe = MilkMonitor_IsMeasurementSafe(now);

    if (measurement_safe) {
        status |= MILK_STATUS_DISTANCE_VALID;
    }
    if (MilkMonitor_IsCalibrated()) {
        status |= MILK_STATUS_CALIBRATED;
    }
    if (pump_requested) {
        status |= MILK_STATUS_PUMP_REQUESTED;
    }
    if (pump_requested && !measurement_safe) {
        status |= MILK_STATUS_FORCED_OFF;
    }

    packet[0] = MILK_TELEMETRY_HEADER;
    packet[1] = MILK_TELEMETRY_VERSION;
    packet[2] = (uint8_t)(distance_mm >> 8);
    packet[3] = (uint8_t)(distance_mm & 0xFFU);
    packet[4] = MilkMonitor_GetLevelPercent(measurement_safe);
    packet[5] = WaterPump_IsOn() ? 1U : 0U;
    packet[6] = status;
    packet[7] = telemetry_sequence++;
    packet[8] = 0U;
    packet[9] = MILK_TELEMETRY_FOOTER;

    (void)HAL_UART_Transmit(&huart1, packet, sizeof(packet), 10U);
}

void MilkMonitor_Init(void)
{
    pump_requested = false;
    distance_valid = false;
    has_measurement = false;
    distance_mm = 0U;
    last_measurement_tick = HAL_GetTick();
    last_telemetry_tick = HAL_GetTick();
    telemetry_sequence = 0U;

    WaterPump_Init();
    Ultrasound_Init();
}

void MilkMonitor_SetPumpRequest(bool requested)
{
    pump_requested = requested;
    if (!requested) {
        WaterPump_Off();
    }
}

bool MilkMonitor_GetPumpRequest(void)
{
    return pump_requested;
}

void MilkMonitor_Update(void)
{
    uint32_t now = HAL_GetTick();
    uint16_t measured_distance_mm;
    bool measured_valid;
    bool measurement_safe;

    Ultrasound_Update();

    if (Ultrasound_TakeMeasurement(&measured_distance_mm, &measured_valid)) {
        distance_mm = measured_valid ? measured_distance_mm : 0U;
        distance_valid = measured_valid;
        has_measurement = true;
        last_measurement_tick = now;
    }

    measurement_safe = MilkMonitor_IsMeasurementSafe(now);
    WaterPump_Set(pump_requested && measurement_safe);

    if ((now - last_telemetry_tick) >= MILK_TELEMETRY_INTERVAL_MS) {
        last_telemetry_tick = now;
        MilkMonitor_SendTelemetry(now);
    }
}
