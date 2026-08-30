/**
 ******************************************************************************
 * @file    I2C.h
 * @brief   I2C1 interface for WT101 migration test
 * @pin_resources PB6=SCL, PB7=SDA; @peripherals I2C1
 * @function Exposes MX_I2C1_Init and hi2c1.
 * @purpose Provides the WT101 sensor bus.
 * @migration Source: src/Core/Inc/i2c.h; adapted from PB8/PB9 to PB6/PB7.
 ******************************************************************************
 */
#ifndef I2C_H
#define I2C_H
#include "stm32f1xx_hal.h"
extern I2C_HandleTypeDef hi2c1;
void MX_I2C1_Init(void);
#endif
