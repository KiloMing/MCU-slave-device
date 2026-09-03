/**
 ******************************************************************************
 * @file    test_motor_encoder.c
 * @brief   Host tests for four-wheel encoder conversion and quadrature decode
 * @pin_resources No physical pins; exercises pure encoder calculations.
 * @peripherals None.
 * @function Verifies the original 10 ms speed formula and GPIO decoding.
 * @purpose Prevents encoder scale or direction logic changing during migration.
 ******************************************************************************
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "MotorEncoder.h"

static void expect_i32(int32_t actual, int32_t expected, const char *message)
{
    if (actual != expected)
    {
        fprintf(stderr, "FAIL: %s actual=%ld expected=%ld\n", message,
                (long)actual, (long)expected);
        exit(1);
    }
}

static void test_original_speed_conversion(void)
{
    expect_i32(MotorEncoder_PulseToSpeed(22), 1600,
               "22 pulses retains original speed result");
    expect_i32(MotorEncoder_PulseToSpeed(-22), -1600,
               "negative pulses retain signed speed result");
    expect_i32(MotorEncoder_PulseToSpeed(0), 0,
               "zero pulses produce zero speed");
}

static void test_quadrature_decode(void)
{
    const uint8_t forward[] = {0U, 1U, 3U, 2U, 0U};
    const uint8_t reverse[] = {0U, 2U, 3U, 1U, 0U};
    int32_t count = 0;
    uint32_t index;

    for (index = 1U; index < 5U; index++)
    {
        count += MotorEncoder_DecodeStep(forward[index - 1U], forward[index]);
    }
    expect_i32(count, 4, "forward quadrature cycle");

    count = 0;
    for (index = 1U; index < 5U; index++)
    {
        count += MotorEncoder_DecodeStep(reverse[index - 1U], reverse[index]);
    }
    expect_i32(count, -4, "reverse quadrature cycle");
    expect_i32(MotorEncoder_DecodeStep(0U, 3U), 0,
               "invalid two-bit jump is ignored");
}

int main(void)
{
    test_original_speed_conversion();
    test_quadrature_decode();
    puts("PASS: four-wheel encoder calculations");
    return 0;
}
