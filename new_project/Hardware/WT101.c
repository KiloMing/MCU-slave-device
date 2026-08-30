/**
 ******************************************************************************
 * @file    WT101.c
 * @brief   WT101 yaw register driver
 * @pin_resources PB6=SCL, PB7=SDA; @peripherals I2C1
 * @function Reads register 0x3F at address 0x50 and converts yaw to degrees.
 * @purpose Independent WT101 migration and bench validation.
 * @migration Source: src/Hardwaer/wt101.c; protocol, byte order and formula unchanged.
 ******************************************************************************
 */
#include "WT101.h"
#include "I2C.h"
#define WT101_ADDR_7BIT 0x50U
#define WT101_YAW_REG  0x3FU
float WT101_ReadYaw(void)
{
    uint8_t data[2] = {0U, 0U};
    uint16_t raw;
    if (HAL_I2C_Mem_Read(&hi2c1, (WT101_ADDR_7BIT << 1), WT101_YAW_REG,
                        I2C_MEMADD_SIZE_8BIT, data, 2U, 100U) != HAL_OK) {
        return 0.0f;
    }
    raw = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    return ((float)raw / 32768.0f) * 180.0f;
}
