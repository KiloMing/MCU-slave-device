// ...existing code...
#include "motor.h"
#include "main.h"        // 包含定时器PWM相关定义
#include "tim.h"
#include "mulun.h"
#include <stdint.h>
#include <stdbool.h>

static PID_Mulun_HandleTypeDef mulun_pid;
/*
  说明（4 路 PWM 映射）：
  - TIM8 通道映射:
      TIM_CHANNEL_1 -> 左前 (LF)
      TIM_CHANNEL_3 -> 右前 (RF)
      TIM_CHANNEL_2 -> 左后 (LB)
      TIM_CHANNEL_4 -> 右后 (RB)
  - speed 带符号表示方向：>0 正转，<0 反转，=0 停转
  - 所有写入 PWM 的值均为非负，且做 Period 限幅保护
*/

/* 内部帮助：获取并限幅为非负 PWM（基于 htim8.Init.Period） */
static uint32_t motor_clamp_pwm(int32_t speed)
{
    uint32_t pwm = (speed < 0) ? (uint32_t)(-speed) : (uint32_t)speed;
    uint32_t period = 0;
    /* 若 htim8 已初始化，则使用其周期作为上限 */
    if (&htim2 != NULL) {
        period = htim2.Init.Period;
    }
    if (period > 0 && pwm > period) pwm = period;
    return pwm;
}

/* 内部帮助：写 PWM 到指定 TIM 通道 */
static void motor_set_channel_pwm(uint32_t channel, uint32_t pwm)
{
    __HAL_TIM_SET_COMPARE(&htim2, channel, pwm);
}

void Motor_Init(void) {
    PID_Mulun_Init(&mulun_pid);
}

/* 将所有单轮 GPIO 方向操作封装，便于整体控制 */
void motor_forward_pin(void)
{
    motor_lf_forward_pin();
    motor_lb_forward_pin();
    motor_rf_forward_pin();
    motor_rb_forward_pin();
}

void motor_back_pin(void)
{
    motor_lf_back_pin();
    motor_lb_back_pin();
    motor_rf_back_pin();
    motor_rb_back_pin();
}

/* 停止所有 PWM 输出（写 0） */
void motor_stop_all(void)
{
    motor_set_channel_pwm(TIM_CHANNEL_1, 0);
    motor_set_channel_pwm(TIM_CHANNEL_2, 0);
    motor_set_channel_pwm(TIM_CHANNEL_3, 0);
    motor_set_channel_pwm(TIM_CHANNEL_4, 0);
}

/**
  * @brief 设置所有电机的速度与方向（统一速度）
  * @param speed: 带符号速度（>0 正转，<0 反转，=0 停转）
  */
void motor_all_set(int32_t speed)
{
    if (speed > 0) {
        motor_forward_pin();
        uint32_t pwm = motor_clamp_pwm(speed);
        motor_set_channel_pwm(TIM_CHANNEL_1, pwm);
        motor_set_channel_pwm(TIM_CHANNEL_2, pwm);
        motor_set_channel_pwm(TIM_CHANNEL_3, pwm);
        motor_set_channel_pwm(TIM_CHANNEL_4, pwm);
    } else if (speed < 0) {
        motor_back_pin();
        uint32_t pwm = motor_clamp_pwm(speed);
        motor_set_channel_pwm(TIM_CHANNEL_1, pwm);
        motor_set_channel_pwm(TIM_CHANNEL_2, pwm);
        motor_set_channel_pwm(TIM_CHANNEL_3, pwm);
        motor_set_channel_pwm(TIM_CHANNEL_4, pwm);
    } else {
        motor_stop_all();
    }
}

void motor_sp(int32_t speed)
{
    if (speed > 0) {
      motor_lf_forward_pin();
			motor_lb_back_pin();
			motor_rf_forward_pin();
			motor_rb_back_pin();
			
        uint32_t pwm = motor_clamp_pwm(speed);
        motor_set_channel_pwm(TIM_CHANNEL_1, pwm);
        motor_set_channel_pwm(TIM_CHANNEL_2, pwm);
        motor_set_channel_pwm(TIM_CHANNEL_3, pwm);
        motor_set_channel_pwm(TIM_CHANNEL_4, pwm);
    } else if (speed < 0) {
			
      motor_lb_forward_pin();
			motor_lf_back_pin();
			motor_rb_forward_pin();
			motor_rf_back_pin();
			
        uint32_t pwm = motor_clamp_pwm(speed);
        motor_set_channel_pwm(TIM_CHANNEL_1, pwm);
        motor_set_channel_pwm(TIM_CHANNEL_2, pwm);
        motor_set_channel_pwm(TIM_CHANNEL_3, pwm);
        motor_set_channel_pwm(TIM_CHANNEL_4, pwm);
    } else {
        motor_stop_all();
    }
}
/**
  * @brief 麦轮横向移动
  * @param speed: 横向速度 (>0 向左, <0 向右)
  */
void motor_strafe(int32_t speed)
{
    if (speed > 0) {  // 向左平移
        motor_lf_forward_pin();  // ↘正转
        motor_lb_back_pin();     // ↗反转  
        motor_rf_back_pin();     // ↙反转
        motor_rb_forward_pin();  // ↖正转
    } else if (speed < 0) {  // 向右平移
        motor_lf_back_pin();     // ↖反转
        motor_lb_forward_pin();  // ↙正转
        motor_rf_forward_pin();  // ↗正转
        motor_rb_back_pin();     // ↘反转
    } else {
        motor_stop_all();
    }
    
    uint32_t pwm = motor_clamp_pwm(speed);
    motor_set_channel_pwm(TIM_CHANNEL_1, pwm);
    motor_set_channel_pwm(TIM_CHANNEL_2, pwm);
    motor_set_channel_pwm(TIM_CHANNEL_3, pwm);
    motor_set_channel_pwm(TIM_CHANNEL_4, pwm);
}


/**
  * @brief 麦轮全向移动
  * @param vx: 前后速度 (>0 前进, <0 后退)
  * @param vy: 横向速度 (>0 左移, <0 右移) 
  * @param omega: 旋转速度 (>0 顺时针, <0 逆时针)
  */

void mecanum_move(int32_t vx, int32_t vy, float omega)
{
    // 运动学分解
    int32_t speed_lf = vx - vy + omega;
    int32_t speed_rf = vx + vy - omega;
    int32_t speed_lb = vx + vy + omega; 
    int32_t speed_rb = vx - vy - omega;
    
    // 设置各轮速度
    Motor_LF_SetSpeed(speed_lf);
    Motor_RF_SetSpeed(speed_rf);
    Motor_LB_SetSpeed(speed_lb);
    Motor_RB_SetSpeed(speed_rb);
}
// 全向移动 + 角度PID校正
void mecanum_with_heading_control(uint16_t vx, uint16_t vy, float target_yaw, float current_yaw)
{
	
	
	// 计算PID校正值
	float pid_data = PID_Mulun_Calc(&mulun_pid, target_yaw, current_yaw);  // 参数名改为 current_yaw
    
    // 设置各轮速度
	mecanum_move(vx,vy,pid_data);
}


/* 单轮速度设置接口 - 对应各自的 TIM 通道与方向 GPIO */
/**
  * @brief  设置右前电机速度（RF -> TIM_CHANNEL_2）
  */
void Motor_RF_SetSpeed(int32_t speed)
{
    if (speed >= 0) {
        motor_rf_forward_pin();
    } else {
        motor_rf_back_pin();
    }
    uint32_t pwm = motor_clamp_pwm(speed);
    motor_set_channel_pwm(TIM_CHANNEL_3, pwm);
}

/**
  * @brief  设置右后电机速度（RB -> TIM_CHANNEL_4）
  */
void Motor_RB_SetSpeed(int32_t speed)
{
    if (speed >= 0) {
        motor_rb_forward_pin();
    } else {
        motor_rb_back_pin();
    }
    uint32_t pwm = motor_clamp_pwm(speed);
    motor_set_channel_pwm(TIM_CHANNEL_4, pwm);
}

/**
  * @brief  设置左前电机速度（LF -> TIM_CHANNEL_1）
  */
void Motor_LF_SetSpeed(int32_t speed)
{
    if (speed >= 0) {
        motor_lf_forward_pin();
    } else {
        motor_lf_back_pin();
    }
    uint32_t pwm = motor_clamp_pwm(speed);
    motor_set_channel_pwm(TIM_CHANNEL_1, pwm);
}

/**
  * @brief  设置左后电机速度（LB -> TIM_CHANNEL_3）
  */
void Motor_LB_SetSpeed(int32_t speed)
{
    if (speed >= 0) {
        motor_lb_forward_pin();
    } else {
        motor_lb_back_pin();
    }
    uint32_t pwm = motor_clamp_pwm(speed);
    motor_set_channel_pwm(TIM_CHANNEL_2, pwm);
}

/**
  * @brief  初始化电机 PWM（启动 TIM8 四个通道）
  */
void motor_PWM_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
}

/* 以下 GPIO 方向控制函数保持原名、实现不变以兼容现有硬件映射 */
/**
  * @brief  设置左前电机为正转方向
  */
void motor_lf_forward_pin(void){
    HAL_GPIO_WritePin(MOTOR_LF2_GPIO_Port, MOTOR_LF2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_LF1_GPIO_Port, MOTOR_LF1_Pin, GPIO_PIN_RESET);
}
void motor_lb_forward_pin(void){
    HAL_GPIO_WritePin(MOTOR_LB2_GPIO_Port, MOTOR_LB2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_LB1_GPIO_Port, MOTOR_LB1_Pin, GPIO_PIN_SET);
}
void motor_rf_forward_pin(void){
    HAL_GPIO_WritePin(MOTOR_RF2_GPIO_Port, MOTOR_RF2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_RF1_GPIO_Port, MOTOR_RF1_Pin, GPIO_PIN_SET);
}
void motor_rb_forward_pin(void){
    HAL_GPIO_WritePin(MOTOR_RB1_GPIO_Port, MOTOR_RB1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_RB2_GPIO_Port, MOTOR_RB2_Pin, GPIO_PIN_RESET);
}
void motor_lf_back_pin(void){
    HAL_GPIO_WritePin(MOTOR_LF1_GPIO_Port, MOTOR_LF1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_LF2_GPIO_Port, MOTOR_LF2_Pin, GPIO_PIN_RESET);
}
void motor_lb_back_pin(void){
    HAL_GPIO_WritePin(MOTOR_LB1_GPIO_Port, MOTOR_LB1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_LB2_GPIO_Port, MOTOR_LB2_Pin, GPIO_PIN_SET);
}
void motor_rf_back_pin(void){
    HAL_GPIO_WritePin(MOTOR_RF1_GPIO_Port, MOTOR_RF1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_RF2_GPIO_Port, MOTOR_RF2_Pin, GPIO_PIN_SET);
}
void motor_rb_back_pin(void){
    HAL_GPIO_WritePin(MOTOR_RB2_GPIO_Port, MOTOR_RB2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_RB1_GPIO_Port, MOTOR_RB1_Pin, GPIO_PIN_RESET);
}
// ...existing code...
