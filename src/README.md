# MCU-slave-device 工程快速掌握说明

> 仓库：`KiloMing/MCU-slave-device`  
> 上游仓库：`Muzhixing/MCU-slave-device`  
> 目标：让第一次接触该工程的人能够快速理解架构、控制链路、通信协议、硬件资源和修改入口。  
> 建议阅读方式：先读本文第 1～7 节，再回到源码按“推荐阅读顺序”逐文件确认。

---

## 1. 一句话认识这个工程

这是一个基于 **STM32F103C8T6 + HAL** 的下位机控制工程。

它目前的核心任务可以概括为：

1. 通过 **USART1** 接收上位机的 10 字节控制指令；
2. 使用 **状态机** 对接收到的数据进行解析和执行；
3. 控制一个 **四轮麦克纳姆底盘**；
4. 通过 **WT101 航向传感器 + PID** 对底盘航向进行闭环修正；
5. 通过 **TIM3 PWM** 控制舵机；
6. 通过 **CAN** 控制两个闭环步进电机机构；
7. 使用 **HC-SR04 超声波**检测液面/距离；
8. 根据上位机水泵请求与超声波有效性控制水泵；
9. 通过 USART1 周期性回传液位遥测数据。

整个工程目前 **没有使用 RTOS**，属于典型的：

```text
中断 + 状态机 + super-loop（while(1)）
```

架构。

---

# 2. 先记住整个系统的数据流

这是掌握本工程最重要的一张图。

```text
                         上位机
                           │
             10 字节控制包 │ USART1
                           ▼
                HAL_UART_RxCpltCallback()
                           │
                           ▼
                    rx_buffer[10]
                           │
                   rx_complete_flag = 1
                           │
                           ▼
                  State_Machine_Update()
                           │
                     STATE_UART_PARSE
                           │
                           ▼
                    UART_Parse_Data()
                           │
                 解析 UART_Packet_t
                           │
                           ▼
                       UART_Launch()
              ┌────────────┼─────────────┐
              │            │             │
              ▼            ▼             ▼
         底盘速度       舵机角度       步进机构
      motor_vx/vy     TIM3 PWM       CAN → Emm_V5
              │
              ▼
      State_Idle_Handler()
              │
              ├── Read_Yaw()
              │      │
              │      ▼
              │    WT101
              │     I2C
              │
              ├── PID_Mulun_Calc()
              │
              ▼
     mecanum_with_heading_control()
              │
              ▼
       mecanum_move()
              │
     ┌────────┼────────┐
     ▼        ▼        ▼
    LF       RF       LB/RB
   TIM2 四路 PWM + GPIO 方向

同时：

 State_Idle_Handler()
          │
          ▼
 MilkMonitor_Update()
          │
          ├── Ultrasound_Update()
          │       │
          │       ▼
          │  HC-SR04 / TIM4 输入捕获
          │
          ├── 水泵安全判断
          │
          ▼
      WaterPump_Set()
          │
          ▼
         PB0

每 200 ms：
MilkMonitor → USART1 → 上位机遥测包
```

理解这张图以后，整个工程基本已经掌握了一半。

---

# 3. MCU 与开发环境

## 3.1 MCU

工程目标芯片：

```text
STM32F103C8T6
Cortex-M3
```

Keil 工程中配置：

```text
Device: STM32F103C8
Flash: 64 KB
RAM:   20 KB
```

系统时钟在 `Core/Src/main.c` 中设置：

```text
HSE
 ↓
PLL × 9
 ↓
SYSCLK = 72 MHz
```

总线：

```text
AHB  = 72 MHz
APB1 = 36 MHz
APB2 = 72 MHz
```

STM32F1 在 APB1 分频不为 1 时，TIM2/TIM3/TIM4 的定时器时钟会得到倍频，因此这些定时器实际工作在 72 MHz 定时器时钟基础上。

---

## 3.2 当前最可靠的构建方式：Keil

仓库中存在：

```text
MDK-ARM/c8t6oled.uvprojx
```

工程配置使用：

```text
Keil µVision 5
ARMCC 5.06
STM32F1xx DFP
HAL
```

仓库保存的 build log 显示：

```text
0 Error(s)
0 Warning(s)
```

程序大小大约：

```text
Code    = 18944 bytes
RO-data = 312 bytes
RW-data = 72 bytes
ZI-data = 1496 bytes
```

### 对新参与者的建议

第一次把工程跑起来时：

> **优先使用 Keil 工程。**

不要先折腾 CMake。

---

## 3.3 CMake 当前并不是完全可靠的主构建配置

根目录虽然存在：

```text
CMakeLists.txt
CMakePresets.json
cmake/
```

但是根 `CMakeLists.txt` 当前只显式加入了部分 `Hardwaer` 模块，例如：

```text
encoder.c
motor.c
usart_parse.c
line_trace.c
water_pump.c
ultrasound.c
milk_monitor.c
```

而实际 Keil 工程还包含：

```text
State_Machine.c
mulun.c
wt101.c
Servo.c
Emm_V5.c
hcan.c
tower.c
...
```

因此：

```text
Keil 工程文件
     ↓
目前更接近“真实可运行配置”

CMakeLists.txt
     ↓
存在源文件列表不同步问题
```

如果你以后准备完善这个仓库，**统一 Keil 与 CMake 的 source list** 是一个很适合作为贡献任务的问题。

---

# 4. 工程目录怎么读

不要逐个阅读所有文件。

推荐把目录理解为：

```text
MCU-slave-device/
│
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── tim.h
│   │   ├── usart.h
│   │   ├── can.h
│   │   └── i2c.h
│   │
│   └── Src/
│       ├── main.c               ← 程序入口
│       ├── gpio.c
│       ├── tim.c
│       ├── usart.c
│       ├── can.c
│       ├── i2c.c
│       └── stm32f1xx_it.c       ← 中断入口
│
├── Hardwaer/                    ← 业务代码最重要的目录
│   ├── hardware.h               ← 业务模块统一头文件
│   │
│   ├── State_Machine.c/.h       ← 整个系统调度核心
│   ├── usart_parse.c/.h         ← 上位机通信协议
│   │
│   ├── motor.c/.h               ← 四轮底盘
│   ├── mulun.c/.h               ← 航向 PID
│   ├── wt101.c/.h               ← IMU/Yaw
│   │
│   ├── Servo.c/.h               ← 舵机
│   │
│   ├── hcan.c/.h                ← CAN 封装
│   ├── Emm_V5.c/.h              ← 闭环步进电机协议
│   ├── tower.c/.h               ← 丝杠/齿轮机构
│   │
│   ├── ultrasound.c/.h          ← HC-SR04
│   ├── water_pump.c/.h          ← 水泵
│   └── milk_monitor.c/.h        ← 液位监测与安全逻辑
│
├── Drivers/
│   ├── CMSIS/
│   └── STM32F1xx_HAL_Driver/
│
├── MDK-ARM/
│   └── c8t6oled.uvprojx         ← Keil 工程
│
├── c8t6oled.ioc                 ← CubeMX 配置
└── CMakeLists.txt
```

注意：

`Hardwaer` 是仓库现有拼写，并不是标准英文 `Hardware`。不要直接改目录名，否则会影响工程文件和 include path。

---

# 5. 程序从哪里开始

入口：

```text
Core/Src/main.c
```

核心流程非常简单。

```c
HAL_Init();

SystemClock_Config();

MX_GPIO_Init();
MX_TIM2_Init();
MX_USART1_UART_Init();
MX_TIM3_Init();
MX_TIM4_Init();
MX_I2C1_Init();
MX_CAN_Init();

HAL_Delay(1000);

State_Machine_Init();

while (1)
{
    State_Machine_Update();
}
```

因此：

```text
main.c
不是业务逻辑中心

State_Machine.c
才是业务逻辑入口
```

你阅读本工程时，读完 `main.c` 以后应该立刻跳到：

```text
Hardwaer/State_Machine.c
```

---

# 6. 状态机：整个工程的调度中心

状态定义：

```c
STATE_INIT
STATE_IDLE
STATE_TASK1
STATE_TASK2
STATE_UART_PARSE
```

目前真正使用的主要是：

```text
STATE_INIT
   ↓
STATE_IDLE
   ↕
STATE_UART_PARSE
```

而：

```text
STATE_TASK1
STATE_TASK2
```

目前只是预留框架。

---

## 6.1 初始化状态

`State_Init_Handler()` 依次执行：

```c
motor_PWM_Init();
Motor_Init();
CAN_Start(CAN_NUM);
UART_Enable_Receive();
Servo_Init();
MilkMonitor_Init();
```

对应：

```text
TIM2 PWM启动
    ↓
航向PID初始化
    ↓
CAN初始化
    ↓
USART1开始中断接收
    ↓
TIM3舵机PWM启动
    ↓
超声波 + 水泵液位模块初始化
```

然后：

```text
STATE_INIT
    ↓
STATE_IDLE
```

---

## 6.2 IDLE 并不是真的“什么都不做”

这是本工程非常重要的一点。

`State_Idle_Handler()` 每次循环都会：

```c
float yaw = Read_Yaw();

mecanum_with_heading_control(
    motor_vx,
    motor_vy,
    target_yaw,
    yaw
);

MilkMonitor_Update();
```

所以 IDLE 实际承担：

```text
底盘实时航向控制
+
液位监控
+
水泵管理
+
等待新串口命令
```

当：

```c
rx_complete_flag == 1
```

时：

```text
STATE_IDLE
    ↓
STATE_UART_PARSE
```

---

# 7. USART1 控制协议 —— 必须掌握

这是你最应该优先掌握的协议。

配置：

```text
USART1
115200 baud
8 data bits
1 stop bit
No parity
No flow control
```

引脚：

```text
PA9  → TX
PA10 → RX
```

使用：

```text
UART 中断
每次接收 1 字节
```

而不是 DMA。

---

## 7.1 上位机 → MCU 控制包

固定长度：

```text
10 bytes
```

格式：

| Byte | 字段 | 含义 |
|---:|---|---|
| 0 | `header` | 固定 `0xB3` |
| 1 | `forward_speed` | 前进速度 |
| 2 | `horizontal_speed` | 横移速度 |
| 3 | `target_angle` | 目标航向角 |
| 4 | `rudder_angle` | 舵机角度 |
| 5 | `lift_rod` | 升降杆位置 |
| 6 | `horizontal_rod` | 水平杆位置 |
| 7 | `switch_one` | 开关 1，目前用于水泵请求 |
| 8 | `switch_two` | 开关 2，目前保留 |
| 9 | `footer` | 固定 `0xB4` |

因此一个包：

```text
B3
│
├── forward_speed
├── horizontal_speed
├── target_angle
├── rudder_angle
├── lift_rod
├── horizontal_rod
├── switch_one
├── switch_two
│
B4
```

---

## 7.2 接收过程

接收函数：

```c
HAL_UART_Receive_IT(&huart1, &rx_data, 1);
```

每接收到 1 字节：

```text
USART1 IRQ
    ↓
HAL_UART_IRQHandler()
    ↓
HAL_UART_RxCpltCallback()
    ↓
rx_buffer[rx_cnt++] = rx_data
```

达到 10 字节以后检查：

```text
长度 = 10
Byte0 = 0xB3
Byte9 = 0xB4
```

有效：

```c
rx_complete_flag = 1;
```

然后暂停继续接收，等待主循环解析。

---

## 7.3 解析之后数据去了哪里

`UART_Parse_Data()` 将 10 字节复制到：

```c
UART_Packet_t rx_packet;
```

之后：

```c
UART_Launch();
```

执行数据。

对应关系：

```text
forward_speed
    ↓
motor_vx

horizontal_speed
    ↓
motor_vy

target_angle
    ↓
target_yaw

rudder_angle
    ↓
Servo_SetAngle_2()

horizontal_rod
    ↓
Gear_StepMotor_ControlByMM()

lift_rod
    ↓
Rail_StepMotor_ControlByMM()

switch_one
    ↓
MilkMonitor_SetPumpRequest()
```

---

# 8. 底盘运动控制

文件：

```text
Hardwaer/motor.c
Hardwaer/mulun.c
Hardwaer/wt101.c
```

这是第二条核心主线。

---

## 8.1 四轮 PWM

底盘使用：

```text
TIM2
```

四个 PWM 通道：

| 轮子 | PWM |
|---|---|
| LF 左前 | TIM2 CH1 |
| LB 左后 | TIM2 CH2 |
| RF 右前 | TIM2 CH3 |
| RB 右后 | TIM2 CH4 |

对应引脚：

```text
PA0 → TIM2_CH1
PA1 → TIM2_CH2
PA2 → TIM2_CH3
PA3 → TIM2_CH4
```

TIM2：

```text
Prescaler = 6
Period    = 999
```

所以 PWM compare 范围：

```text
0 ~ 999
```

---

## 8.2 电机方向不是靠负 PWM

方向由 GPIO 控制。

例如：

```c
motor_lf_forward_pin();
motor_lf_back_pin();
```

逻辑是：

```text
speed > 0
    ↓
设置方向 GPIO 为正转
    ↓
PWM = abs(speed)

speed < 0
    ↓
设置方向 GPIO 为反转
    ↓
PWM = abs(speed)
```

因此：

> **正负号表示方向，PWM compare 永远是非负数。**

---

## 8.3 麦克纳姆运动学

核心：

```c
mecanum_move(vx, vy, omega);
```

计算：

```text
LF = vx - vy + omega
RF = vx + vy - omega
LB = vx + vy + omega
RB = vx - vy - omega
```

含义：

```text
vx     → 前后运动
vy     → 左右平移
omega  → 原地旋转/航向修正
```

因此：

```text
        vy
        ↑
        │
        │
←──── Robot ────→
        │
        │
        ↓
        vx

omega = 绕自身旋转
```

---

# 9. 航向闭环 PID

主循环持续读取：

```c
Read_Yaw();
```

然后：

```c
PID_Mulun_Calc(
    target_yaw,
    current_yaw
);
```

PID 输出不是直接作为整车 PWM。

它作为：

```text
omega
```

进入麦轮运动学：

```text
vx
vy
PID输出 omega
      ↓
mecanum_move()
```

这意味着：

> 上位机负责告诉 MCU“我要往哪里移动、希望车头保持什么角度”，STM32 自己根据当前 Yaw 自动修正四轮速度。

---

## 9.1 PID 默认参数

`Hardwaer/mulun.c`：

```text
P = 12.0
I = 2.0
D = 2.0

积分限幅 = ±35
输出限幅 = ±350
积分分离阈值 = ±30°
```

并且做了角度归一化：

```text
任意角度
    ↓
[-180°, +180°]
```

例如：

```text
目标 = 10°
当前 = 350°

错误不是：
10 - 350 = -340°

而是：
+20°
```

这是航向控制中正确的思路。

---

# 10. WT101 航向传感器

连接：

```text
I2C1
PB8 → SCL
PB9 → SDA
```

I2C：

```text
100 kHz
7-bit address
```

代码使用：

```text
设备地址 = 0x50
Yaw寄存器 = 0x3F
```

读取：

```c
HAL_I2C_Mem_Read()
```

然后将两个字节转换为 Yaw。

所以底盘闭环链路是：

```text
WT101
  │
 I2C
  ↓
Read_Yaw()
  ↓
current_yaw
  ↓
PID
  ↓
omega
  ↓
四轮速度
```

---

# 11. 舵机控制

文件：

```text
Hardwaer/Servo.c
```

使用：

```text
TIM3
```

TIM3 配置：

```text
Prescaler = 71
Period = 19999
```

因此得到典型：

```text
20 ms PWM周期
50 Hz
```

通道：

```text
TIM3 CH1 → PA6
TIM3 CH2 → PA7
```

工程当前 UART 控制实际调用：

```c
Servo_SetAngle_2(rx_packet.rudder_angle);
```

`Servo_SetAngle_2()`：

```text
0°   → 500 us
180° → 2500 us
```

---

# 12. CAN 与闭环步进电机

这部分文件关系：

```text
Core/Src/can.c
       │
       ▼
Hardwaer/hcan.c
       │
       ▼
Hardwaer/Emm_V5.c
       │
       ▼
Hardwaer/tower.c
```

不要直接从 `Emm_V5.c` 第一行开始硬啃。

先理解分层。

---

## 12.1 CAN 底层

CAN1：

```text
PA11 → CAN RX
PA12 → CAN TX
```

配置参数：

```text
Prescaler = 4
BS1 = 13 TQ
BS2 = 4 TQ
SJW = 1 TQ
```

在当前 36 MHz APB1 条件下，对应约：

```text
500 kbit/s
```

---

## 12.2 hcan.c 的作用

`CAN_Start()`：

```text
配置 Filter
    ↓
允许 FIFO0
    ↓
启动 CAN
```

过滤器当前基本等价于：

```text
接收所有 ID
```

`can_SendCmd()`：

负责把：

```text
Emm_V5 指令
```

封装成：

```text
CAN Extended ID 数据帧
```

发出去。

所以：

```text
Emm_V5
不是 CAN 驱动

它只是“步进电机协议层”
```

真正向 HAL CAN 发数据的是：

```text
hcan.c
```

---

# 13. 丝杠和水平机构

文件：

```text
Hardwaer/tower.c
Hardwaer/tower.h
```

这个模块把：

```text
毫米 mm
```

转换成：

```text
电机脉冲数
```

然后调用：

```c
Emm_V5_Pos_Control();
```

---

## 13.1 丝杠

参数：

```text
3200 pulse / revolution
螺距 = 4 mm / revolution
```

因此：

```text
800 pulse/mm
```

函数：

```c
Rail_StepMotor_ControlByMM(...)
```

代码注释约定：

```text
地址 1 → 丝杠机构
```

---

## 13.2 齿轮机构

参数：

```text
齿轮直径 ≈ 40 mm
周长 ≈ 125.66 mm
3200 pulse / revolution
```

函数：

```c
Gear_StepMotor_ControlByMM(...)
```

约定：

```text
地址 2 → 齿轮机构
```

---

## 13.3 一个很重要的优化

`UART_Launch()` 保存：

```c
last_packet
```

只有位置变化时才发新的 CAN 命令：

```text
新 horizontal_rod
        │
        ├── 与旧值相同 → 不发送
        │
        └── 与旧值不同 → CAN发送
```

同样适用于：

```text
lift_rod
```

这样可以防止主循环反复向步进电机发送相同位置指令。

---

# 14. 超声波液位监测

文件：

```text
Hardwaer/ultrasound.c
```

硬件思路类似 HC-SR04。

引脚：

```text
PB6 → TRIG
PB7 → ECHO / TIM4 CH2
```

---

## 14.1 TIM4

TIM4：

```text
Prescaler = 71
Period = 65535
```

即：

```text
计数分辨率 ≈ 1 us
```

ECHO 使用：

```text
TIM4 CH2 Input Capture
```

---

## 14.2 测量状态机

超声波模块内部自己还有一个小状态机：

```text
ULTRASOUND_IDLE
      ↓
ULTRASOUND_WAIT_RISING
      ↓
ULTRASOUND_WAIT_FALLING
      ↓
ULTRASOUND_CAPTURE_COMPLETE
      ↓
ULTRASOUND_IDLE
```

收到上升沿：

```text
记录 rising count
```

然后把捕获极性改成下降沿。

收到下降沿：

```text
记录 falling count
```

得到：

```text
echo_pulse_us
```

最后换算：

```text
distance_mm
```

---

## 14.3 当前参数

有效范围：

```text
20 mm ~ 4000 mm
```

触发周期：

```text
60 ms
```

Echo timeout：

```text
35 ms
```

---

# 15. 水泵与液位安全逻辑

这是最新集成版本里比较完整的一块。

文件：

```text
milk_monitor.c
water_pump.c
ultrasound.c
```

上位机控制包：

```text
switch_one
```

不会直接：

```text
GPIO → 水泵
```

而是：

```text
switch_one
     ↓
MilkMonitor_SetPumpRequest()
     ↓
pump_requested
```

真正打开水泵时：

```c
WaterPump_Set(
    pump_requested &&
    measurement_safe
);
```

也就是同时要求：

```text
上位机请求开泵
      AND
超声波测量有效
```

---

## 15.1 超声波失效保护

测量有效条件：

```text
已经取得过测量值
AND
本次测量有效
AND
测量数据年龄 <= 250 ms
```

否则：

```text
WaterPump OFF
```

所以这个模块已经具备：

```text
传感器失效 → 强制关泵
```

的 fail-safe 思路。

---

# 16. MCU → 上位机遥测协议

`MilkMonitor` 每：

```text
200 ms
```

通过 USART1 发一个 10 字节包。

格式：

| Byte | 内容 |
|---:|---|
| 0 | `0xB5` |
| 1 | 协议版本 `0x01` |
| 2 | distance high |
| 3 | distance low |
| 4 | 液位百分比 |
| 5 | 水泵实际状态 |
| 6 | status bitfield |
| 7 | sequence |
| 8 | reserved |
| 9 | `0xB6` |

即：

```text
B5
│
├── version
├── distance_H
├── distance_L
├── level %
├── pump state
├── status
├── sequence
├── reserved
│
B6
```

---

## 16.1 status

bit 定义：

```text
bit0 → DISTANCE_VALID
bit1 → CALIBRATED
bit2 → PUMP_REQUESTED
bit3 → FORCED_OFF
```

这样上位机不仅知道：

```text
“水泵现在开没开”
```

还可以知道：

```text
“为什么没有开”
```

这比只回一个 pump 状态合理很多。

---

# 17. 当前液位百分比其实还没有完成标定

`milk_monitor.h`：

```c
#define MILK_FULL_DISTANCE_MM  0U
#define MILK_EMPTY_DISTANCE_MM 0U
```

因此目前：

```text
液位百分比 = 0xFF
```

表示：

```text
未标定
```

后续必须实际测量容器：

```text
满液位时：
传感器 → 液面的距离

空容器时：
传感器 → 底部的距离
```

然后设置：

```c
MILK_FULL_DISTANCE_MM
MILK_EMPTY_DISTANCE_MM
```

才能正确得到：

```text
0 ~ 100 %
```

---

# 18. GPIO / 外设总表

## 通信

| 功能 | 外设 | 引脚 |
|---|---|---|
| 上位机串口 TX | USART1 | PA9 |
| 上位机串口 RX | USART1 | PA10 |
| CAN RX | CAN1 | PA11 |
| CAN TX | CAN1 | PA12 |
| WT101 SCL | I2C1 | PB8 |
| WT101 SDA | I2C1 | PB9 |

## 底盘

| 功能 | 引脚 |
|---|---|
| LF PWM | PA0 / TIM2_CH1 |
| LB PWM | PA1 / TIM2_CH2 |
| RF PWM | PA2 / TIM2_CH3 |
| RB PWM | PA3 / TIM2_CH4 |
| LF DIR | PB11 / PB12 |
| LB DIR | PB13 / PB14 |
| RF DIR | PB15 / PA8 |
| RB DIR | PB3 / PB4 |

## 舵机

| 功能 | 引脚 |
|---|---|
| Servo 1 | PA6 / TIM3_CH1 |
| Servo 2 | PA7 / TIM3_CH2 |

## 液位与水泵

| 功能 | 引脚 |
|---|---|
| Water Pump | PB0 |
| Ultrasound TRIG | PB6 |
| Ultrasound ECHO | PB7 / TIM4_CH2 |

---

# 19. 中断体系

当前真正值得关注：

```text
USART1_IRQn
    ↓
HAL_UART_IRQHandler()
    ↓
HAL_UART_RxCpltCallback()
```

用于控制包接收。

```text
TIM4_IRQn
    ↓
HAL_TIM_IRQHandler()
    ↓
HAL_TIM_IC_CaptureCallback()
```

用于 HC-SR04 Echo 测量。

CAN 也配置了 RX interrupt，不过当前工程主要用途是向步进电机发送命令。

---

# 20. 你应该按什么顺序读代码

强烈不建议：

```text
打开工程
↓
从 Core/Src 第一份文件开始逐个阅读
```

推荐：

## 第一轮：理解架构

只读：

```text
1. Core/Src/main.c
2. Hardwaer/State_Machine.c
3. Hardwaer/usart_parse.c
```

目标：

> 搞懂“程序怎么运行”和“上位机命令怎么进入系统”。

---

## 第二轮：理解底盘

```text
4. Hardwaer/motor.c
5. Hardwaer/mulun.c
6. Hardwaer/wt101.c
```

目标：

> 搞懂 `vx / vy / target_yaw` 最后怎样变成四个电机的 PWM。

---

## 第三轮：理解机械执行机构

```text
7. Hardwaer/tower.c
8. Hardwaer/Emm_V5.c
9. Hardwaer/hcan.c
10. Core/Src/can.c
```

目标：

> 搞懂 UART 中的两个杆位置怎样变成 CAN 步进电机命令。

---

## 第四轮：理解液位系统

```text
11. Hardwaer/milk_monitor.c
12. Hardwaer/ultrasound.c
13. Hardwaer/water_pump.c
```

目标：

> 搞懂为什么 `switch_one = 1` 不一定意味着水泵一定会开。

---

## 第五轮：最后才看底层配置

```text
14. Core/Src/tim.c
15. Core/Src/gpio.c
16. Core/Src/i2c.c
17. Core/Src/usart.c
18. Core/Src/stm32f1xx_it.c
```

这些是为了回答：

```text
哪个外设？
哪个通道？
哪个引脚？
什么中断？
什么频率？
```

不是为了理解业务逻辑。

---

# 21. 哪些文件你目前可以先忽略

仓库中还存在：

```text
Core/Src/OLED.c
Core/Src/OLED_Data.c
Core/Src/gray_sensor.c
Core/Src/pid.c
Hardwaer/line_trace.c
Hardwaer/encoder.c
Hardwaer/soft_uart.c
Hardwaer/PID.c
...
```

它们不属于当前 Keil 主控制链路的核心。

尤其：

```text
mulun.c
```

才是当前底盘航向控制真正使用的 PID。

所以一开始不要因为看见：

```text
PID.c
gray_sensor.c
line_trace.c
OLED.c
```

就花大量时间分析。

---

# 22. 当前工程最值得注意的问题

下面不是说工程“不能运行”。

仓库保存的 Keil build log 已经显示：

```text
0 errors
0 warnings
```

这里讨论的是：

```text
架构风险
协议风险
可维护性
鲁棒性
```

---

## P0/P1：CAN RX 中断配置值得立即核对

`can.c` 中启用的是：

```c
CAN1_RX0_IRQn
```

并且 Filter 使用：

```text
FIFO0
```

但当前 `stm32f1xx_it.c` 中看到的是：

```c
CAN1_RX1_IRQHandler()
```

如果未来 CAN 节点产生 FIFO0 接收中断，这两个配置并不匹配。

建议检查并统一：

```text
FIFO0 → CAN1_RX0_IRQHandler
```

如果项目完全只发 CAN、不接收，则影响暂时较小；但一旦驱动器有接收数据，这会成为高优先级问题。

---

## P1：UART 控制协议的速度字段全部是无符号 8 bit

当前：

```c
uint8_t forward_speed;
uint8_t horizontal_speed;
```

因此协议原生范围：

```text
0 ~ 255
```

然而：

```c
mecanum_move(int32_t vx, int32_t vy, ...)
```

底层明显支持：

```text
负速度
```

这意味着当前协议很难直接表达：

```text
后退
右移
```

除非上位机和 MCU 另外约定了编码方式，但当前源码中没有看到相应解码。

以后如果要完善协议，建议考虑：

```text
int8_t
```

或者：

```text
int16_t
```

的有符号速度字段。

---

## P1：target_angle 只有 8 bit

当前：

```c
uint8_t target_angle;
```

范围：

```text
0 ~ 255
```

而航向通常可能需要：

```text
0 ~ 359°
```

或：

```text
-180 ~ +180°
```

因此协议层应重新确认目标航向编码方式。

---

## P1：PID 没有固定采样周期 dt

现在 PID 在：

```text
while(1)
→ STATE_IDLE
```

中被不断调用。

但每轮耗时不是固定的，因为里面存在：

```text
I2C读取
MilkMonitor_Update()
UART状态切换
CAN发送
HAL_Delay()
```

当前 PID：

```text
integral += error
derivative = error - last_error
```

没有使用：

```text
dt
```

因此：

> PID 参数与主循环运行频率强绑定。

如果循环频率变化，PID 动态响应也会变化。

更规范的方案是：

```text
固定 5 ms / 10 ms / 20 ms 控制周期
```

或使用：

```text
dt
```

进行积分和微分计算。

---

## P1：Read_Yaw() 使用 HAL_MAX_DELAY

当前：

```c
HAL_I2C_Mem_Read(..., HAL_MAX_DELAY);
```

也就是说如果 WT101/I2C 出现异常，理论上主循环可能长时间阻塞。

而主循环还负责：

```text
底盘
UART解析
液位监控
水泵安全
```

所以这里更适合：

```text
有限 timeout
+
失败状态处理
```

---

## P1：Yaw 读取失败直接返回 0°

现在：

```text
I2C失败
    ↓
return 0.0f
```

系统无法区分：

```text
Yaw真的等于0°
```

和：

```text
传感器读取失败
```

如果目标角度不是 0°，可能导致错误的航向修正。

更合理的是：

```text
bool Read_Yaw(float *yaw)
```

或：

```text
返回状态 + 输出参数
```

---

## P1：UART 包缺少 checksum / CRC

目前只验证：

```text
长度
包头 0xB3
包尾 0xB4
```

中间 8 字节即使传输出错，也可能被当成有效命令。

对于运动设备，建议未来加入：

```text
CRC8
```

或至少：

```text
checksum
```

---

## P1：UART 丢字节后的重新同步能力较弱

当前逻辑基本是：

```text
收满10字节
↓
检查头尾
↓
失败就整个清零
```

它没有做：

```text
扫描下一个 0xB3
```

因此如果中途丢 1 byte，协议恢复能力有限。

可改为：

```text
状态机接收

WAIT_HEADER
    ↓
RECEIVE_PAYLOAD
    ↓
CHECK_FOOTER / CRC
```

---

## P2：motor.c 中有历史注释没有同步

顶部注释写：

```text
TIM8
```

实际使用：

```text
TIM2
```

这会让新参与者产生误解。

代码本身使用的是：

```c
htim2
```

应该以代码与 `tim.c` 为准。

---

## P2：motor_clamp_pwm() 中判断没有意义

存在：

```c
if (&htim2 != NULL)
```

`htim2` 是全局对象：

```text
&htim2
```

正常情况下永远不可能是 NULL。

这个判断可以简化。

---

## P2：CMake 与 Keil 工程文件不同步

这是当前仓库协作层面最明显的问题之一。

如果团队有人：

```text
Keil
```

有人：

```text
VS Code + CMake + ARM GCC
```

就可能出现：

```text
A能编译
B不能编译
```

建议最终统一 source list。

---

## P2：液位标定参数还是 0

```text
MILK_FULL_DISTANCE_MM  = 0
MILK_EMPTY_DISTANCE_MM = 0
```

因此液位百分比功能当前处于：

```text
未标定
```

状态。

---

## P2：头文件耦合比较严重

`hardware.h` 一次性 include 了很多模块：

```text
motor
Servo
CAN
Emm
tower
PID
State Machine
WT101
ultrasound
water pump
milk monitor
...
```

同时部分模块自己的 `.h` 又 include `hardware.h`。

虽然 include guard 可以避免无限递归，但长期维护容易造成：

```text
强耦合
编译依赖膨胀
循环 include
```

未来可以逐渐改成：

```text
每个模块只 include 自己真正需要的头文件
```

---

# 23. 我认为当前工程的模块成熟度

可以粗略分为：

## A：主流程核心

```text
main.c
State_Machine
usart_parse
motor
mulun
wt101
```

必须掌握。

## B：执行机构

```text
Servo
tower
Emm_V5
hcan
CAN
```

需要理解接口和数据流，不需要一开始背协议。

## C：液位系统

```text
milk_monitor
ultrasound
water_pump
```

结构相对独立，适合作为一个完整模块来理解。

## D：历史/备用代码

```text
OLED
gray_sensor
line_trace
encoder
soft_uart
旧 PID
```

暂时不用优先投入时间。

---

# 24. 以后想改某个功能，应该改哪里

## 修改上位机命令

看：

```text
usart_parse.h
usart_parse.c
```

---

## 修改底盘运动方式

看：

```text
motor.c
```

---

## 调航向 PID

看：

```text
mulun.c
```

主要参数：

```text
PID_MULUN_DEFAULT_P
PID_MULUN_DEFAULT_I
PID_MULUN_DEFAULT_D
```

---

## 修改 WT101

看：

```text
wt101.c
wt101.h
i2c.c
```

---

## 修改舵机角度

看：

```text
Servo.c
tim.c
```

---

## 修改丝杠/水平机构

看：

```text
tower.c
Emm_V5.c
hcan.c
```

---

## 修改液位检测

看：

```text
milk_monitor.c
ultrasound.c
```

---

## 修改水泵安全规则

优先改：

```text
milk_monitor.c
```

不要直接在很多地方：

```c
HAL_GPIO_WritePin(WATER_PUMP...)
```

否则会破坏统一的安全控制。

---

## 修改引脚

首先：

```text
c8t6oled.ioc
```

然后确认：

```text
main.h
gpio.c
tim.c
usart.c
can.c
i2c.c
```

---

# 25. 推荐调试断点

第一次上板，建议按这个顺序打断点。

### 断点 1

```c
State_Machine_Init()
```

确认程序正常启动。

### 断点 2

```c
State_Init_Handler()
```

确认外设业务层初始化执行。

### 断点 3

```c
HAL_UART_RxCpltCallback()
```

上位机发送一个字节，看 MCU 能否收到。

### 断点 4

```c
UART_Parse_Data()
```

发送完整：

```text
0xB3 ... 0xB4
```

确认数据包进入解析。

### 断点 5

```c
UART_Launch()
```

观察：

```text
rx_packet
motor_vx
motor_vy
target_yaw
```

### 断点 6

```c
mecanum_with_heading_control()
```

观察：

```text
target_yaw
current_yaw
pid_data
```

### 断点 7

```c
can_SendCmd()
```

移动升降杆/水平杆时检查 CAN 数据。

### 断点 8

```c
HAL_TIM_IC_CaptureCallback()
```

检查超声波 Echo。

### 断点 9

```c
MilkMonitor_Update()
```

观察：

```text
distance_mm
distance_valid
pump_requested
WaterPump_IsOn()
```

---

# 26. 第一次实际跑工程的安全顺序

不要第一次上电就让所有执行机构一起动。

推荐：

```text
阶段 1
MCU + ST-Link
确认烧录运行

阶段 2
USART1
确认 B3...B4 控制包

阶段 3
WT101
确认 Yaw 数据

阶段 4
底盘悬空
单独确认四轮方向

阶段 5
低 PWM 测试麦轮运动

阶段 6
舵机

阶段 7
CAN + 步进电机

阶段 8
HC-SR04

阶段 9
水泵控制
```

这样一旦出错，很容易判断问题在哪一层。

---

# 27. 60～90 分钟快速掌握路线

如果你今天只想快速把工程吃透，不需要一天全部阅读。

## 0～10 min

读：

```text
main.c
State_Machine.c
```

回答：

```text
程序入口是什么？
while(1)实际执行什么？
有哪些状态？
```

---

## 10～25 min

读：

```text
usart_parse.h
usart_parse.c
```

自己画出：

```text
B3 + 8 bytes + B4
```

并记住：

```text
每一个 byte 控制什么
```

---

## 25～45 min

读：

```text
motor.c
mulun.c
wt101.c
```

必须能够口头解释：

```text
target_yaw
   ↓
PID
   ↓
omega
   ↓
麦轮四轮速度
```

---

## 45～60 min

读：

```text
tower.c
Emm_V5.c
hcan.c
```

只搞懂：

```text
位置值
↓
毫米→脉冲
↓
Emm协议
↓
CAN
```

不用背所有 Emm_V5 命令。

---

## 60～75 min

读：

```text
milk_monitor.c
ultrasound.c
water_pump.c
```

重点搞懂：

```text
为什么上位机请求开泵以后
水泵不一定真的开
```

---

## 75～90 min

读：

```text
tim.c
gpio.c
usart.c
i2c.c
can.c
```

最后建立：

```text
模块 ↔ 外设 ↔ 引脚
```

映射。

---

# 28. 你掌握工程以后应该能回答的 15 个问题

1. `main()` 初始化完成以后为什么几乎没有业务代码？
2. 状态机为什么是整个程序的业务入口？
3. `STATE_IDLE` 为什么不是真正的空闲？
4. USART1 一次接收几个字节？
5. 完整控制包有多少个字节？
6. `0xB3` 和 `0xB4` 是什么？
7. `forward_speed` 最后赋给谁？
8. `target_angle` 如何影响四个底盘电机？
9. WT101 使用哪个外设？
10. 四个底盘电机分别对应 TIM2 哪个 Channel？
11. 舵机为什么使用 20 ms PWM？
12. 升降杆为什么最终走 CAN？
13. `Emm_V5.c` 与 `hcan.c` 的职责有什么区别？
14. 水泵为什么不是直接由 `switch_one` 控制？
15. 当前液位百分比为什么可能一直得到 `0xFF`？

如果这 15 个问题你都可以自己解释：

> 你已经具备开始修改这个工程的能力。

---

# 29. 最建议你先做的贡献任务

如果这是你第一次参与上游项目，不建议第一步重构整个工程。

推荐顺序：

### 任务 A：补 README / 工程文档

风险最低。

本文本身就可以作为基础。

### 任务 B：修正文档和代码中的 TIM8/TIM2 历史注释

小而明确。

### 任务 C：同步 Keil 与 CMake source list

非常适合学习 GitHub 协作。

### 任务 D：核对 CAN FIFO0 / RX0 IRQ

这是有实际技术价值的问题。

### 任务 E：改进 UART 帧同步与 CRC

需要与上位机协议共同修改，适合熟悉工程以后再做。

### 任务 F：让底盘 PID 运行在固定控制周期

属于控制系统层面的改进，最后做。

---

# 30. GitHub 协作关系

你的仓库是 Fork：

```text
Muzhixing/MCU-slave-device
           ↑
        upstream

       你的电脑

           ↓
         origin
           ↓
KiloMing/MCU-slave-device
```

推荐本地：

```bash
git clone https://github.com/KiloMing/MCU-slave-device.git
cd MCU-slave-device

git remote add upstream https://github.com/Muzhixing/MCU-slave-device.git
git remote -v
```

以后：

```text
upstream/main
     ↓
同步原作者代码

自己的 feature branch
     ↓
修改 + 测试

origin/feature-xxx
     ↓
Pull Request

Muzhixing/MCU-slave-device
```

不要直接在自己本地的 `main` 上长期开发功能。

推荐：

```bash
git switch main
git fetch upstream
git merge upstream/main

git switch -c docs/project-guide
```

或：

```bash
git switch -c fix/can-rx-interrupt
```

---

# 31. 最终心智模型

最后不要把这个工程记成几十个 `.c` 文件。

只记住下面六层：

```text
┌─────────────────────────────┐
│          上位机              │
│      发送运动控制命令         │
└──────────────┬──────────────┘
               │ USART1
               ▼
┌─────────────────────────────┐
│       通信/状态机层           │
│ usart_parse + State_Machine │
└──────────────┬──────────────┘
               │
        ┌──────┼─────────┐
        ▼      ▼         ▼
┌──────────┐ ┌──────┐ ┌──────────┐
│ 麦轮底盘  │ │ 舵机 │ │ 步进机构  │
│ motor    │ │Servo │ │ tower    │
│ mulun PID│ │TIM3  │ │ Emm/CAN  │
└────┬─────┘ └──────┘ └──────────┘
     │
     ▼
┌──────────┐
│  WT101   │
│ I2C/Yaw  │
└──────────┘

同时：

┌─────────────┐
│ MilkMonitor │
└──────┬──────┘
       │
   ┌───┴────┐
   ▼        ▼
HC-SR04    水泵
TIM4 IC    PB0
   │
   └──────► USART1 遥测
```

这就是整个 `MCU-slave-device` 当前版本最核心的架构。

---

# 32. 最后给接手者的建议

第一次接手不要追求：

```text
“每一行代码我都懂”
```

真正的目标应该是：

```text
我知道程序从哪里开始
+
我知道数据从哪里进入
+
我知道数据经过哪些模块
+
我知道最后控制了什么硬件
+
我知道修改某个功能应该去哪个文件
+
我知道哪些代码现在值得警惕
```

做到这些，你就已经从：

```text
“看别人代码”
```

进入：

```text
“可以参与这个工程”
```

的阶段。
