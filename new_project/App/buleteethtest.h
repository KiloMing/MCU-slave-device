/**
 ******************************************************************************
 * @file    buleteethtest.h
 * @brief   Jiangxie Bluetooth four-wheel speed PID chassis interface
 * @pin_resources USART2 PA2/PA3; PC13 LED; four motor/encoder pins unchanged.
 * @peripherals USART2, TIM1-TIM4, GPIOA/GPIOB/GPIOC, EXTI and SysTick.
 * @function Parses [j,LX,LY,RX,RY] into normalized Mecanum speed targets.
 * @purpose Controls translation and rotation through four encoder PID loops.
 * @migration Retains joystick axes and original Mecanum wheel equations.
 ******************************************************************************
 */

#ifndef BULETEETHTEST_H
#define BULETEETHTEST_H

#include <stdint.h>

typedef struct
{
    int32_t left_x;
    int32_t left_y;
    int32_t right_x;
    int32_t right_y;
} BluetoothTest_Joystick_t;

uint8_t BluetoothTest_ParseJoystick(const char *packet,
                                    BluetoothTest_Joystick_t *joystick);
void BluetoothTest_MapTranslation(const BluetoothTest_Joystick_t *joystick,
                                  int32_t *vx, int32_t *vy);
float BluetoothTest_MapRotation(int32_t left_x);
void BluetoothTest_CalculateWheelTargets(int32_t vx, int32_t vy, float omega,
                                         int32_t targets[4]);
void BluetoothTest_Init(void);
void BluetoothTest_RunStep(void);

#endif
