/**
  **************************************************************************************************
  * @file    mulun.c
  * @author  Auto Generated
  * @version V1.0.0
  * @date    2024
  * @brief   麦克纳姆轮角度PID控制器实现文件
  * 
  * @details 本文件实现了用于麦克纳姆轮全向移动机器人的角度PID控制器，主要功能包括：
  *          - 角度360度环绕处理（自动将角度归一化到[-180, 180]范围）
  *          - 积分分离功能（大误差时暂停积分，防止积分饱和导致超调）
  *          - 积分限幅和输出限幅保护
  *          - 支持多实例的PID控制器设计
  * 
  * @note    本控制器专为STM32C8T6单片机设计，用于控制麦克纳姆轮机器人的航向角稳定。
  **************************************************************************************************
  */

#include "mulun.h"

/* *************************************************************************************************
 * PID控制器默认参数配置
 * *************************************************************************************************/
#define PID_MULUN_DEFAULT_P       12.0f    /* 比例系数(P)：控制响应速度，增大P可加快响应，但过大易振荡 */
#define PID_MULUN_DEFAULT_I       2.0f    /* 积分系数(I)：消除稳态误差，增大I可加快积分，但易超调 */
#define PID_MULUN_DEFAULT_D       2.0f    /* 微分系数(D)：抑制振荡，增大D可减小超调，但易引入噪声 */
#define PID_MULUN_DEFAULT_I_MAX   35.0f   /* 积分上限：防止积分项过大导致积分饱和 */
#define PID_MULUN_DEFAULT_OUT_MAX 350.0f  /* 输出上限：限制PID输出的最大幅值，保护电机 */
#define PID_MULUN_INTEG_SEP_THRES 30.0f   /* 积分分离阈值：角度误差超过此值时暂停积分 */

/* *************************************************************************************************
 * 内部静态函数声明与实现
 * *************************************************************************************************/

/**
  * @brief  角度规范化函数（将任意角度转换到[-180°, 180°]范围）
  * 
  * @param  angle: 原始角度值（单位：度）
  * @retval 规范化后的角度值（范围：-180° ~ +180°）
  * 
  * @note   角度控制中必须处理360度环绕问题。例如：
  *         - 当目标角度为10°，当前角度为350°时，实际误差应为20°而非-340°
  *         - 本函数通过循环加减360°实现角度归一化
  */
static inline float normalize_angle(float angle) {
    /* 使用while循环处理超出范围的角度 */
    while (angle > 180.0f) angle -= 360.0f;  /* 角度大于180°时减去360° */
    while (angle < -180.0f) angle += 360.0f; /* 角度小于-180°时加上360° */
    return angle;
}

/* *************************************************************************************************
 * 外部接口函数实现
 * *************************************************************************************************/

/**
  * @brief  初始化PID控制器
  * 
  * @param  hpid: PID控制器句柄指针（PID_Mulun_HandleTypeDef结构体指针）
  * @retval None
  * 
  * @note   初始化内容包括：
  *         1. 将PID参数(P/I/D)设置为默认值
  *         2. 设置积分上限和输出上限
  *         3. 清零积分项和上次误差值
  *         4. 空指针检查，增强代码健壮性
  */
void PID_Mulun_Init(PID_Mulun_HandleTypeDef *hpid) {
    /* 空指针检查：防止传入NULL导致程序崩溃 */
    if (hpid == NULL) return;
    
    /* 设置默认PID参数 */
    hpid->p = PID_MULUN_DEFAULT_P;
    hpid->i = PID_MULUN_DEFAULT_I;
    hpid->d = PID_MULUN_DEFAULT_D;
    
    /* 设置限幅参数 */
    hpid->integral_max = PID_MULUN_DEFAULT_I_MAX;
    hpid->output_max = PID_MULUN_DEFAULT_OUT_MAX;
    
    /* 清零控制器状态 */
    hpid->integral = 0.0f;
    hpid->last_error = 0.0f;
}

/**
  * @brief  计算PID输出（角度控制专用）
  * 
  * @param  hpid: PID控制器句柄指针
  * @param  target_yaw: 目标角度（单位：度）
  * @param  current_yaw: 当前角度（单位：度，由传感器读取）
  * @retval PID输出值（用于角度校正的PWM值，范围：-output_max ~ +output_max）
  * 
  * @note   PID计算流程：
  *         1. 角度规范化处理（处理360°环绕）
  *         2. 计算角度误差
  *         3. 积分计算（带积分分离功能）
  *         4. 微分计算（基于误差变化率）
  *         5. 合成PID输出
  *         6. 输出限幅保护
  */
float PID_Mulun_Calc(PID_Mulun_HandleTypeDef *hpid, float target_yaw, float current_yaw) {
    /* 空指针检查：防止传入NULL导致程序崩溃 */
    if (hpid == NULL) return 0.0f;
    
    /* 步骤1：角度规范化处理 */
    float current = normalize_angle(current_yaw);  /* 规范化当前角度 */
    float error = normalize_angle(target_yaw - current);  /* 计算规范化后的角度误差 */
    
    /* 步骤2：积分计算（带积分分离功能） */
    if (hpid->i != 0.0f) {  /* 仅当积分系数不为零时进行积分 */
        /* 积分分离条件：只有当误差在阈值范围内时才进行积分 */
        /* 目的：大误差时暂停积分，防止积分饱和导致超调 */
        if (error > -PID_MULUN_INTEG_SEP_THRES && error < PID_MULUN_INTEG_SEP_THRES) {
            hpid->integral += error;  /* 累加误差到积分项 */
            
            /* 积分限幅：防止积分项过大 */
            if (hpid->integral > hpid->integral_max) {
                hpid->integral = hpid->integral_max;      /* 正方向限幅 */
            } else if (hpid->integral < -hpid->integral_max) {
                hpid->integral = -hpid->integral_max;     /* 负方向限幅 */
            }
        }
    }
    
    /* 步骤3：微分计算 */
    float derivative = error - hpid->last_error;  /* 计算误差变化率（微分项） */
    hpid->last_error = error;                    /* 更新上次误差值，供下次计算使用 */
    
    /* 步骤4：PID输出合成 */
    /* PWM = P*error + D*derivative + I*integral */
    float pwm = hpid->p * error + hpid->d * derivative + hpid->i * hpid->integral;
    
    /* 步骤5：输出限幅保护 */
    if (pwm > hpid->output_max) {
        pwm = hpid->output_max;      /* 正方向限幅 */
    } else if (pwm < -hpid->output_max) {
        pwm = -hpid->output_max;     /* 负方向限幅 */
    }
//    printf("PID Output: %.2f\n", pwm);
    /* 返回PID输出值（用于控制电机进行角度校正） */
    return pwm;
}

/***************************************** END OF FILE *****************************************/
