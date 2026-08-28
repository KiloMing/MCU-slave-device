#ifndef __EMM_V5_H
#define __EMM_V5_H

#include "can.h"
#include "hcan.h"
#include "stdint.h"
#include "stdbool.h"

/**********************************************************
 * Emm_V5 驱动 - CAN 命令封装（头文件）
 * 作者: ZHANGDATOU (原作者信息保留)
 * 说明:
 *  - 本文件声明了与 Emm_V5 系列驱动器通信的常用接口（通过 CAN 总线发送命令）。
 *  - 所有命令函数会组包并调用底层 Can_SendCmd / CAN 发送函数。
 *  - 使用前请确保 CAN 总线已初始化且可发送。
 **********************************************************/

#define ABS(x) ((x) > 0 ? (x) : -(x))

/* 要读取的系统参数类型（用于 Emm_V5_Read_Sys_Params） */
typedef enum {
    S_VER   = 0,   /* 读取固件/模块版本 */
    S_RL    = 1,   /* 读取回路/限位相关参数 */
    S_PID   = 2,   /* 读取 PID 参数 */
    S_VBUS  = 3,   /* 读取总线电压 */
    S_CPHA  = 5,   /* 读取相位/相序设置 */
    S_ENCL  = 7,   /* 读取编码器/位置校准相关 */
    S_TPOS  = 8,   /* 读取目标位置/位置误差 */
    S_VEL   = 9,   /* 读取实时速度 */
    S_CPOS  = 10,  /* 读取当前位置 */
    S_PERR  = 11,  /* 读取位置误差或误差累计 */
    S_FLAG  = 13,  /* 读取状态标志（使能/限位/转向等） */
    S_Conf  = 14,  /* 读取配置信息 */
    S_State = 15,  /* 读取系统运行状态 */
    S_ORG   = 16,  /* 读取原点/归零相关状态 */
} SysParams_t;

/**********************************************************
 * 接口声明（按功能分组）
 * 备注: 所有函数均以 CAN 命令形式发送到指定驱动器地址 addr。
 **********************************************************/

/* 基本控制 */
void Emm_V5_Reset_CurPos_To_Zero(uint8_t addr); /* 将当前位置复位为 0（写入/更新驱动器内部位置） */
void Emm_V5_Reset_Clog_Pro(uint8_t addr);       /* 复位限位/堵转保护状态或计数 */

/* 参数读取与修改 */
void Emm_V5_Read_Sys_Params(uint8_t addr, SysParams_t s); /* 读取指定系统参数 */
void Emm_V5_Modify_Ctrl_Mode(uint8_t addr, bool svF, uint8_t ctrl_mode); /* 修改并可选择保存控制模式（svF=true 保存） */

/* 使能 / 输出控制 */
void Emm_V5_En_Control(uint8_t addr, bool state, bool snF); /* 使能或禁止驱动输出，snF 为同步标志 */

/* 运动控制 */
void Emm_V5_Vel_Control(uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc, bool snF); /* 速度控制，vel 单位为 RPM，acc 为加速参数 */
void Emm_V5_Pos_Control(uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc, uint32_t clk, bool raF, bool snF); /* 位置控制：clk 为目标计数，raF=绝对/相对，snF=同步标志 */
void Emm_V5_Stop_Now(uint8_t addr, bool snF); /* 紧急停止（立即停止），可同步 */

/* 同步与原点相关 */
void Emm_V5_Synchronous_motion(uint8_t addr); /* 触发已设置的同步动作（用于多驱动同步启动） */
void Emm_V5_Origin_Set_O(uint8_t addr, bool svF); /* 设置当前点为原点，svF=true 时写入非易失存储 */
void Emm_V5_Origin_Modify_Params(uint8_t addr, bool svF, uint8_t o_mode, uint8_t o_dir, uint16_t o_vel, uint32_t o_tm, uint16_t sl_vel, uint16_t sl_ma, uint16_t sl_ms, bool potF); /* 修改原点搜索与限位复位参数 */
void Emm_V5_Origin_Trigger_Return(uint8_t addr, uint8_t o_mode, bool snF); /* 触发原点返回动作（根据 o_mode ） */
void Emm_V5_Origin_Interrupt(uint8_t addr); /* 示例：原点中断/使能相关命令 */

#endif
