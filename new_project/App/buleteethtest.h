/**
 ******************************************************************************
 * @file    buleteethtest.h
 * @brief   Jiangxie Bluetooth joystick open-loop chassis test interface
 * @pin_resources USART2 PA2/PA3; PC13 external LED; motor pins unchanged.
 * @peripherals USART2, TIM1, TIM2, GPIOA/GPIOB/GPIOC and SysTick.
 * @function Parses [j,LX,LY,RX,RY] and maps it directly to vx, vy and omega.
 * @purpose Tests forward, backward, lateral and turning motion without WT101.
 * @migration Reuses the original mecanum_move() calculation unchanged.
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
void BluetoothTest_Init(void);
void BluetoothTest_RunStep(void);

#endif
