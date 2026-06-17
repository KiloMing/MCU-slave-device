#include "tower.h"


/**
 * @brief  丝杠步进电机控制移动指定距离，调用Emm_V5_Pos_Control
 * @param  addr: 设备地址（如0x01等）
 * @param  move_mm: 移动距离（mm，正数=正方向，负数=反方向）
 * @param  vel: 速度分辩率0~255，值越大速度越快
 * @param  acc: 加速度分辩率0~255，值越大加速度越快
 * @param  raF: 相对/绝对位置，true=相对，false=绝对
 * @param  snF: 同步标志，true=等待同步，false=立即执行
 * @retval 无
 */
void Rail_StepMotor_ControlByMM( uint8_t move_mm,uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc, bool raF, bool snF) {
   if (move_mm>180)
   {
       move_mm = 180;
   }
   
    // 1. 计算丝杠步进电机所需脉冲数，使用PULSE_PER_MM
    uint32_t pulse = (uint32_t)(move_mm * PULSE_PER_MM);  
    // 3. 调用位置控制函数
    Emm_V5_Pos_Control(addr, dir, vel, acc, pulse, raF, snF);
}

/**
 * @brief  齿轮步进电机控制移动指定距离，调用Emm_V5_Pos_Control
 * @param  addr: 设备地址（如0x02等）
 * @param  move_mm: 移动距离（mm，正数=正方向，负数=反方向）
 * @param  vel: 速度分辩率0~255，值越大速度越快
 * @param  acc: 加速度分辩率0~255，值越大加速度越快
 * @param  raF: 相对/绝对位置，true=相对，false=绝对
 * @param  snF: 同步标志，true=等待同步，false=立即执行
 * @retval 无
 */
void Gear_StepMotor_ControlByMM( uint8_t move_mm,uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc, bool raF, bool snF) {

        if(move_mm>180){
        move_mm = 180;
				}
    // 2. 计算齿轮步进电机所需脉冲数，根据齿轮周长转换
    uint32_t pulse = (uint32_t)(move_mm * STEP_MOTOR_PULSE_PER_CIRCLE / GEAR_CIRCUMFERENCE + 0.5f);
    
    // 4. 调用位置控制函数
    Emm_V5_Pos_Control(addr, dir, vel, acc, pulse, raF, snF);
}
