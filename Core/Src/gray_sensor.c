#include "gray_sensor.h"
#include "tim.h"

/**
 * @brief 简单读取1路灰度传感器串行数据（8bit）
 * @retval 读取到的8位灰度值
 */
unsigned char gw_gray_serial_read_simple(void)
{
    unsigned char ret = 0;
    int i;
    
    for (i = 0; i < 8; i++)
    {
        // 时钟下降沿
        HAL_GPIO_WritePin(GW_GRAY_GPIO_PORT, GW_GRAY_GPIO_CLK_PIN, GPIO_PIN_RESET);
        
        // 读取数据位，并左移放到 ret 对应位
        if (HAL_GPIO_ReadPin(GW_GRAY_GPIO_PORT, GW_GRAY_GPIO_DAT_PIN) == GPIO_PIN_SET)
        {
            ret |= (1 << i);
        }
        
        // 时钟上升沿
        HAL_GPIO_WritePin(GW_GRAY_GPIO_PORT, GW_GRAY_GPIO_CLK_PIN, GPIO_PIN_SET);
        
        // 延时 5us
        delay_us(5);
    }
    
    // 读取完8位后，延时1ms进行"归零"
    HAL_Delay(1);
    
    return ret;
}
