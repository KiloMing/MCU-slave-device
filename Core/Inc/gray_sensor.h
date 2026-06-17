#ifndef GRAY_SENSOR_H
#define GRAY_SENSOR_H

#include "main.h"

// 定义引脚（你可以根据实际硬件修改）
#define GW_GRAY_GPIO_CLK_PIN GPIO_PIN_4
#define GW_GRAY_GPIO_DAT_PIN GPIO_PIN_5
#define GW_GRAY_GPIO_PORT GPIOB

/**
 * @brief 简单读取1路灰度传感器串行数据（8bit）
 * @retval 读取到的8位灰度值
 */
unsigned char gw_gray_serial_read_simple(void);

#endif /* GRAY_SENSOR_H */
