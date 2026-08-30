/**
 ******************************************************************************
 * @file    WT101.h
 * @brief   WT101 yaw readout interface
 * @pin_resources PB6=SCL, PB7=SDA; @peripherals I2C1
 * @function Reads the two-byte yaw register.
 * @purpose Supplies heading feedback for later control migration.
 * @migration Source: src/Hardwaer/wt101.h; bus pins adapted only.
 ******************************************************************************
 */
#ifndef WT101_H
#define WT101_H
#include "stdint.h"
float WT101_ReadYaw(void);
#endif
