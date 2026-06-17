/******************************************************************************
 * PID.c
 *
 * 功能：基于 PID 算法的两轴舵机控制模块（X/Y）
 *
 * 说明与用途：
 *  - pid_S_Y 控制 Y 轴（映射到 270° 舵机），pid_S_X 控制 X 轴（映射到 180° 舵机）。
 *  - 本文件新增对“上电默认角度为 0°”的支持：在系统初始化阶段调用 PID_SetPowerOnZero()
 *    可将两路舵机移动到角度 0（即约定的中立位），并设置 now_x/now_y 作为初始值。
 *  - PID 输出通过中点基准 ks1/ks2_1 映射为微秒脉宽，并进行限幅保护。
 *
 * 上电默认角度说明（用途与作用）：
 *  - “默认上电角度为 0”表示设备上电初始化后，舵机应移动到 0°（中立位置），以保证机械初始姿态一致，便于后续控制。
 *  - PID_SetPowerOnZero() 的作用：在主程序初始化完成后调用，立即下发中点脉宽到舵机，避免电机漂移或未知初始角度带来的误动作。
 *
 ******************************************************************************/


#include "PID.h"

/* ---------------- 舵机映射常量 ---------------- */
/* Y 轴 270° 舵机（微秒） */
#define Y_SERVO_MIN_US  500U
#define Y_SERVO_MAX_US  2500U
#define Y_SERVO_ANGLE_MAX 270U

/* X 轴 180° 舵机（微秒） */
#define X_SERVO_MIN_US  500U
#define X_SERVO_MAX_US  2500U
#define X_SERVO_ANGLE_MAX 180U

/* 中心基准（微秒），中立位（角度 0°）默认置为中点 */
/* 若需要把 0° 定义为其他角度，请调整 ks1/ks2_1 的值 */
uint16_t ks1 = (Y_SERVO_MIN_US + Y_SERVO_MAX_US) / 2;    // Y 轴（270° 舵机）中点（微秒）
uint16_t ks2_1 = (X_SERVO_MIN_US + X_SERVO_MAX_US) / 2;  // X 轴（180° 舵机）中点（微秒）——保留原名以兼容原代码

/* ---------------- 全局误差与输出变量 ---------------- */
uint16_t Err_X = 0;
uint16_t Err_Y = 0;

/* PID 输出暂存（会取整为微秒偏移） */
int16_t x_pwm = 0;     // X 轴 PID 输出（用于映射到微秒）
int16_t now_x;         // X 轴最终下发微秒值
int16_t y_pwm = 0;     // Y 轴 PID 输出
int16_t now_y;         // Y 轴最终下发微秒值

/* ---------------- Y 轴 PID 状态（浮点用于积分/微分计算） ---------------- */
float Err_S_Y = 0.0f;
float last_Err_S_Y = 0.0f;
float integral = 0.0f;

/* Y 轴 PID 参数（可根据需要调整） */
float p_S = -0.88f;
float i_S = -0.00001f;
float d_S = -0.41f;

/* ---------------- X 轴 PID 状态 ---------------- */
float Err_S_X = 0.0f;
float last_Err_S_X = 0.0f;
float integral_X = 0.0f;

/* X 轴 PID 参数 */
float p_S_X = -0.398f;
float i_S_X = -0.00001f;
float d_S_X = -0.24999f;

/**
 * PID_SetPowerOnZero
 * 作用：在系统初始化阶段调用，使两路舵机移动到上电默认角度 0°（中立位）并设置 now_x/now_y 。
 * 说明与用途：
 *  - 保证机械初始化姿态一致，避免未知上电角度导致的碰撞或大幅误差。
 *  - 建议在 MX 外设初始化后、主循环与 PID 调用前执行一次。
 */
void PID_SetPowerOnZero(void)
{
    now_y = ks1;
    now_x = ks2_1;

    /* 下发中点脉宽到舵机（Servo_SetAngle_* 假定接收微秒脉宽） */
    Servo_SetAngle_2(now_y);
    Servo_SetAngle_1(now_x);

    /* 可选显示或延时，帮助确认动作已完成（不强制阻塞） */
 
}

/**
 * Y 轴 PID 控制器 —— 目标控制 270° 舵机
 */
void pid_S_Y(float true_S, float tar_S)
{
    /* 1. 误差 */
    Err_S_Y = tar_S - true_S;
    
    /* 2. 积分累加（简单积分） */
    integral += Err_S_Y;
    
    /* 3. PID 计算（得到控制量，按微秒偏移近似映射） */
    y_pwm = (uint16_t)(p_S * Err_S_Y + i_S * integral + d_S * (Err_S_Y - last_Err_S_Y));
    
    /* 4. 保存当前误差用于下一次微分计算 */
    last_Err_S_Y = Err_S_Y;
    
    /* 5. 将 PID 输出映射为舵机微秒值（以中点 ks1 为基准） */
    int32_t tmp = (int32_t)ks1 - (int32_t)y_pwm; /* 减号保留原控制方向逻辑 */
    
    /* 6. 限幅到 Y 舵机支持的微秒范围 */
    if (tmp > (int32_t)Y_SERVO_MAX_US) tmp = (int32_t)Y_SERVO_MAX_US;
    if (tmp < (int32_t)Y_SERVO_MIN_US) tmp = (int32_t)Y_SERVO_MIN_US;
    now_y = (int16_t)tmp;
    
    /* 7. 下发到对应舵机接口（保持原名 Servo_SetAngle_2） */
    Servo_SetAngle_2(now_y);

}

/**
 * X 轴 PID 控制器 —— 目标控制 180° 舵机
 */
void pid_S_X(float true_S, float tar_S)
{
    /* 1. 误差 */
    Err_S_X = tar_S - true_S;
    
    /* 2. 积分累加 */
    integral_X += Err_S_X;
    
    /* 3. PID 计算 */
    x_pwm = (int16_t)(p_S_X * Err_S_X + i_S_X * integral_X + d_S_X * (Err_S_X - last_Err_S_X));
    
    /* 4. 保存当前误差 */
    last_Err_S_X = Err_S_X;
    
    /* 5. 对 PID 输出限幅（保留一个小范围以避免大幅超出） */
    if (x_pwm > 1000) {
        x_pwm = 1000;
    } else if (x_pwm < -1000) {
        x_pwm = -1000;
    }

    /* 6. 将 PID 输出映射为舵机微秒值（以 ks2_1 为中点） */
    int32_t tmp = (int32_t)ks2_1 - (int32_t)x_pwm;
    
    /* 7. 限幅到 X 舵机支持的微秒范围 */
    if (tmp > (int32_t)X_SERVO_MAX_US) tmp = (int32_t)X_SERVO_MAX_US;
    if (tmp < (int32_t)X_SERVO_MIN_US) tmp = (int32_t)X_SERVO_MIN_US;
    now_x = (int16_t)tmp;
    
    /* 8. 下发与显示（保持原下发函数名 Servo_SetAngle_1） */
    Servo_SetAngle_1(now_x);

}






























