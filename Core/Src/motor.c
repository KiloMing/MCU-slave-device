#include "motor.h"
#include "tim.h"        // 包含定时器PWM相关定义

/**
  * @brief  设置所有电机的速度和方向
  * @param  speed: 电机速度值，范围通常为0~最大PWM值
  *                正值表示正转，负值表示反转
  * @retval 无
  */
void motor_all_set(int32_t speed){

    if (speed > 0){
        motor_forward_pin();
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, speed);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, speed);
    } else if (speed < 0) { // 仅负速度时切换反转
        motor_back_pin();
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, speed);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, speed);
    } else { // speed=0，仅停转，不改变方向
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
    }                    
}


/**
  * @brief  设置右电机的速度和方向
  * @param  speed: 右电机速度值，正值正转，负值反转
  * @retval 无
  */
void Motor_R_SetSpeed(int32_t speed){
    // 正值为正转
    if (speed >= 0)
    {
        motor_r_forward_pin();  // 设置右电机为正转方向
        
        // 设置右电机PWM（TIM2通道2）
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, speed);
    }
    // 负值为反转
    else
    {
        motor_r_back_pin();     // 设置右电机为反转方向
        
        // 取绝对值设置PWM
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, -speed);
    }
}


/**
  * @brief  设置左电机的速度和方向
  * @param  speed: 左电机速度值，正值正转，负值反转
  * @retval 无
  */
void Motor_L_SetSpeed(int32_t speed){
    // 正值为正转
    if (speed >= 0)
    {
        motor_l_forward_pin();  // 设置左电机为正转方向
        
        // 设置左电机PWM（TIM2通道1）
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, speed);
    }
    // 负值为反转
    else
    {
        motor_l_back_pin();     // 设置左电机为反转方向
        
        // 取绝对值设置PWM
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, -speed);
    }
}

/**
  * @brief  初始化电机PWM输出
  * @note   启动TIM2的通道1和通道2的PWM输出功能
  * @param  无
  * @retval 无
  */
void motor_PWM_Init(void){

    // 启动TIM2通道1的PWM输出（左电机）
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    // 启动TIM2通道2的PWM输出（右电机）
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}


/**
  * @brief  设置所有电机为正转方向
  * @note   通过控制电机驱动芯片的方向引脚实现
  * @param  无
  * @retval 无
  */
void motor_forward_pin(void){
    // 左电机正转：L2=高电平，L1=低电平
    HAL_GPIO_WritePin(motor_l2_GPIO_Port, motor_l2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(motor_l1_GPIO_Port, motor_l1_Pin, GPIO_PIN_RESET);	
	
    // 右电机正转：R2=高电平，R1=低电平
    HAL_GPIO_WritePin(motor_r2_GPIO_Port, motor_r2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(motor_r1_GPIO_Port, motor_r1_Pin, GPIO_PIN_RESET);	
}

/**
  * @brief  设置所有电机为反转方向
  * @note   通过控制电机驱动芯片的方向引脚实现
  * @param  无
  * @retval 无
  */
void motor_back_pin(void){
    // 左电机反转：L1=高电平，L2=低电平
    HAL_GPIO_WritePin(motor_l1_GPIO_Port, motor_l1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(motor_l2_GPIO_Port, motor_l2_Pin, GPIO_PIN_RESET);	
	                
    // 右电机反转：R1=高电平，R2=低电平
    HAL_GPIO_WritePin(motor_r1_GPIO_Port, motor_r1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(motor_r2_GPIO_Port, motor_r2_Pin, GPIO_PIN_RESET);
}

/**
  * @brief  设置左电机为正转方向
  * @param  无
  * @retval 无
  */
void motor_l_forward_pin(void){
    // 左电机正转：L2=高电平，L1=低电平
    HAL_GPIO_WritePin(motor_l2_GPIO_Port, motor_l2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(motor_l1_GPIO_Port, motor_l1_Pin, GPIO_PIN_RESET);	
}

/**
  * @brief  设置右电机为正转方向
  * @param  无
  * @retval 无
  */
void motor_r_forward_pin(void){
    // 右电机正转：R2=高电平，R1=低电平
    HAL_GPIO_WritePin(motor_r2_GPIO_Port, motor_r2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(motor_r1_GPIO_Port, motor_r1_Pin, GPIO_PIN_RESET);	
}

/**
  * @brief  设置左电机为反转方向
  * @param  无
  * @retval 无
  */
void motor_l_back_pin(void){
    // 左电机反转：L1=高电平，L2=低电平
    HAL_GPIO_WritePin(motor_l1_GPIO_Port, motor_l1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(motor_l2_GPIO_Port, motor_l2_Pin, GPIO_PIN_RESET);	
}

/**
  * @brief  设置右电机为反转方向
  * @param  无
  * @retval 无
  */
void motor_r_back_pin(void){
    // 右电机反转：R1=高电平，R2=低电平
    HAL_GPIO_WritePin(motor_r1_GPIO_Port, motor_r1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(motor_r2_GPIO_Port, motor_r2_Pin, GPIO_PIN_RESET);		
}
