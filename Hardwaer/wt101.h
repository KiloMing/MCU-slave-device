#ifndef __WT101_H
#define __WT101_H
#include "main.h"
#include "stdint.h"
#include "i2c.h"

// 设备默认I2C地址（由IICADDR寄存器默认值0x0050提取低7位：0x50）
#define DEFAULT_DEV_ADDR 0x50
// Yaw偏航角寄存器地址
#define YAW_REG_ADDR     0x3F

/**
  * @brief  读取 WT101 传感器的 Yaw 角度
  * @param  yaw_angle: 存储 Yaw 角度的指针
  * @retval 0: 成功, 1: 参数错误, 2: 读取失败
  */
float Read_Yaw(void);

#endif /* __WT101_H */
