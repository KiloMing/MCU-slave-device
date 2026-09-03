/**
 ******************************************************************************
 * @file    MotorEncoderMath.c
 * @brief   Hardware-independent encoder conversion and quadrature decoding
 * @pin_resources No direct pins; used by PB6/PB7, PA6/PA7, PB0/PB1, PA5/PA12.
 * @peripherals None.
 * @function Converts 10 ms pulse counts and validates quadrature transitions.
 * @purpose Keeps the migrated four-encoder calculations host-testable.
 * @migration Formula and constants are unchanged from the original encoder.c.
 ******************************************************************************
 */

#include "MotorEncoder.h"

int32_t MotorEncoder_PulseToSpeed(int32_t pulse_count)
{
    return (int32_t)(((int64_t)pulse_count * 60000LL *
                      MOTOR_ENCODER_SPEED_SCALE) /
                     (MOTOR_ENCODER_RESOLUTION * MOTOR_ENCODER_GEAR_RATIO *
                      MOTOR_ENCODER_SAMPLE_PERIOD_MS));
}

int8_t MotorEncoder_DecodeStep(uint8_t previous_state, uint8_t current_state)
{
    static const int8_t transition[16] = {
         0,  1, -1,  0,
        -1,  0,  0,  1,
         1,  0,  0, -1,
         0, -1,  1,  0
    };
    uint8_t index = (uint8_t)(((previous_state & 0x03U) << 2U) |
                              (current_state & 0x03U));
    return transition[index];
}
