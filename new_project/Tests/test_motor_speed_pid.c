/**
 ******************************************************************************
 * @file    test_motor_speed_pid.c
 * @brief   Host behavior tests for four independent motor speed PID loops
 * @pin_resources No physical pins; models LF/LB/RF/RB motor outputs.
 * @peripherals Models the 1 ms HAL SysTick software scheduling source.
 * @function Verifies parameters, 10 ms scheduling, limiting and four outputs.
 * @purpose Protects the original incremental PID data during four-wheel porting.
 ******************************************************************************
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "MotorSpeedPID.h"

static uint32_t fake_tick;
static uint32_t update_count[4];
static int32_t motor_output[4];

uint32_t HAL_GetTick(void)
{
    return fake_tick;
}

void Motor_LF_SetSpeed(int32_t speed)
{
    update_count[0]++;
    motor_output[0] = speed;
}

void Motor_LB_SetSpeed(int32_t speed)
{
    update_count[1]++;
    motor_output[1] = speed;
}

void Motor_RF_SetSpeed(int32_t speed)
{
    update_count[2]++;
    motor_output[2] = speed;
}

void Motor_RB_SetSpeed(int32_t speed)
{
    update_count[3]++;
    motor_output[3] = speed;
}

static void fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
}

static void expect_i32(int32_t actual, int32_t expected, const char *message)
{
    if (actual != expected)
    {
        fprintf(stderr, "FAIL: %s actual=%ld expected=%ld\n", message,
                (long)actual, (long)expected);
        exit(1);
    }
}

static void expect_four_outputs(int32_t lf, int32_t lb,
                                int32_t rf, int32_t rb)
{
    expect_i32(motor_output[0], lf, "LF PID output");
    expect_i32(motor_output[1], lb, "LB PID output");
    expect_i32(motor_output[2], rf, "RF PID output");
    expect_i32(motor_output[3], rb, "RB PID output");
}

static void test_original_parameters_and_software_period(void)
{
    const int32_t targets[4] = {50, 100, 150, 400};
    const int32_t measured[4] = {0, 0, 0, 0};

    if ((MOTOR_SPEED_PID_DEFAULT_KP != 2) ||
        (MOTOR_SPEED_PID_DEFAULT_KI != 1) ||
        (MOTOR_SPEED_PID_DEFAULT_KD != 0) ||
        (MOTOR_SPEED_PID_PERIOD_MS != 10U) ||
        (MOTOR_SPEED_PID_OUTPUT_MAX != 699))
    {
        fail("original speed PID data or requested output limit changed");
    }

    fake_tick = 100U;
    MotorSpeedPID_Init();
    MotorSpeedPID_SetTargets(targets);
    MotorSpeedPID_SetMeasuredSpeeds(measured);

    if (MotorSpeedPID_RunStep() != 0U)
    {
        fail("PID ran immediately instead of waiting for 10 ms");
    }
    fake_tick = 109U;
    if (MotorSpeedPID_RunStep() != 0U)
    {
        fail("PID ran before the 10 ms software period");
    }
    fake_tick = 110U;
    if (MotorSpeedPID_RunStep() == 0U)
    {
        fail("PID did not run at the 10 ms software period");
    }

    expect_four_outputs(150, 300, 450, 699);
    expect_i32((int32_t)update_count[0], 1, "LF updated once");
    expect_i32((int32_t)update_count[1], 1, "LB updated once");
    expect_i32((int32_t)update_count[2], 1, "RF updated once");
    expect_i32((int32_t)update_count[3], 1, "RB updated once");
}

static void test_stop_reset_and_signed_reverse_output(void)
{
    int32_t reverse_target[4] = {-50, 0, 0, 0};
    const int32_t stopped_feedback[4] = {0, 0, 0, 0};

    MotorSpeedPID_SetTargets(reverse_target);
    MotorSpeedPID_SetMeasuredSpeeds(stopped_feedback);
    fake_tick = 120U;
    if (MotorSpeedPID_RunStep() == 0U)
    {
        fail("reverse PID update did not run");
    }
    expect_four_outputs(-150, 0, 0, 0);

    reverse_target[0] = 0;
    MotorSpeedPID_SetTargets(reverse_target);
    fake_tick = 130U;
    (void)MotorSpeedPID_RunStep();
    expect_four_outputs(0, 0, 0, 0);
}

static void test_individual_motor_target_api_and_observation(void)
{
    const int32_t measured[4] = {10, 20, 30, 40};

    fake_tick = 200U;
    MotorSpeedPID_Init();
    Motor_A_SetTargetSpeed(100);
    Motor_B_SetTargetSpeed(200);
    Motor_C_SetTargetSpeed(300);
    Motor_D_SetTargetSpeed(400);
    MotorSpeedPID_SetMeasuredSpeeds(measured);

    expect_i32(MotorSpeedPID_GetTarget(0U), 100, "Motor A target API");
    expect_i32(MotorSpeedPID_GetTarget(1U), 200, "Motor B target API");
    expect_i32(MotorSpeedPID_GetTarget(2U), 300, "Motor C target API");
    expect_i32(MotorSpeedPID_GetTarget(3U), 400, "Motor D target API");
    expect_i32(MotorSpeedPID_GetMeasuredSpeed(0U), 10,
               "Motor A measured speed API");

    fake_tick = 210U;
    (void)MotorSpeedPID_RunStep();
    expect_i32(MotorSpeedPID_GetOutput(0U), 270,
               "Motor A observable PID output");

    Motor_A_SetTargetSpeed(0);
    Motor_B_SetTargetSpeed(0);
    Motor_C_SetTargetSpeed(0);
    Motor_D_SetTargetSpeed(0);
}

static void test_positive_target_never_reverses_on_overspeed(void)
{
    const int32_t target[4] = {100, 0, 0, 0};
    const int32_t stopped[4] = {0, 0, 0, 0};
    const int32_t overspeed[4] = {1000, 0, 0, 0};

    fake_tick = 300U;
    MotorSpeedPID_Init();
    MotorSpeedPID_SetTargets(target);
    MotorSpeedPID_SetMeasuredSpeeds(stopped);
    fake_tick = 310U;
    (void)MotorSpeedPID_RunStep();
    expect_i32(MotorSpeedPID_GetOutput(0U), 300,
               "positive target starts with positive PWM");

    MotorSpeedPID_SetMeasuredSpeeds(overspeed);
    fake_tick = 320U;
    (void)MotorSpeedPID_RunStep();
    expect_i32(MotorSpeedPID_GetOutput(0U), 0,
               "overspeed clamps PWM at zero instead of reversing");
}

static void test_scaled_p_gain_and_feedforward_for_motor_b(void)
{
    const int32_t stopped[4] = {0, 0, 0, 0};
    const int32_t at_target[4] = {0, 1600, 0, 0};

    fake_tick = 400U;
    MotorSpeedPID_Init();
    MotorSpeedPID_SetParameters(1U, 5, 0, 0, 100, 540);
    Motor_B_SetTargetSpeed(1600);
    MotorSpeedPID_SetMeasuredSpeeds(stopped);

    fake_tick = 410U;
    (void)MotorSpeedPID_RunStep();
    expect_i32(MotorSpeedPID_GetOutput(1U), 620,
               "B starts from feedforward plus Kp=0.05 correction");

    MotorSpeedPID_SetMeasuredSpeeds(at_target);
    fake_tick = 420U;
    (void)MotorSpeedPID_RunStep();
    expect_i32(MotorSpeedPID_GetOutput(1U), 540,
               "B returns to feedforward at target speed");
}

int main(void)
{
    test_original_parameters_and_software_period();
    test_stop_reset_and_signed_reverse_output();
    test_individual_motor_target_api_and_observation();
    test_positive_target_never_reverses_on_overspeed();
    test_scaled_p_gain_and_feedforward_for_motor_b();
    puts("PASS: four independent motor speed PID software loops");
    return 0;
}
