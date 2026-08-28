#include "wt101.h"

/**
  * @brief  读取 WT101 传感器的 Yaw 角度
  * @param  yaw_angle: 存储 Yaw 角度的指针
  * @retval 0: 成功, 1: 参数错误, 2: 读取失败
  */
float Read_Yaw(void) {
    uint8_t reg_data[2];  // 存储 Yaw 角度的高低字节
    uint16_t yaw_raw;     // 16位原始数据
    HAL_StatusTypeDef status;

    // 2. 读取 Yaw 角度寄存器（2字节）
    status = HAL_I2C_Mem_Read(&hi2c1, DEFAULT_DEV_ADDR << 1, YAW_REG_ADDR, I2C_MEMADD_SIZE_8BIT, reg_data, 2, HAL_MAX_DELAY);
    if (status != HAL_OK) {
        return 0.0f;  // 读取失败
    }

    // 3. 组合16位原始数据（注意字节顺序，高位在前，低位在后）
    yaw_raw = (uint16_t)(reg_data[1] << 8) | reg_data[0];

    // 4. 转换为实际角度（公式：Yaw[15:0]/32768 * 180度）
    return (float)yaw_raw / 32768.0f * 180.0f;
}

















