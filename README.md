# STM32F103C8T6 驱动 28BYJ-48 步进电机（ULN2003）

本工程使用 STM32F103C8T6 通过 ULN2003 达林顿管阵列驱动 28BYJ-48 五线四相步进电机，
实现正反转的精确步数控制。

## 硬件连接

| 28BYJ-48 / ULN2003 | STM32F103C8T6 |
| ------------------ | ------------- |
| IN1                | PA0           |
| IN2                | PA1           |
| IN3                | PA2           |
| IN4                | PA3           |
| VCC (5V)           | 外部 5V 电源   |
| GND                | 共地（与 MCU 共地） |

注意 28BYJ-48 工作电流较大，必须由 ULN2003 模块上的 5V 输入供电，
不能直接由 STM32 的 3.3V 引脚带载。MCU 与电机驱动板必须共地。

## 控制原理

### 电机参数

- 28BYJ-48 内部减速比 1/64，电机相数 4，步距角 5.625°/64
- 采用八拍（半步）驱动方式时，转一圈需要 `64 × 64 = 4096` 步
- 在 `stepmotor.h` 中定义为 `STEPS_PER_REV = 4096`

### 八拍时序

`stepmotor.c` 中 `StepSeq[8]` 保存了 IN4–IN1 的输出电平序列（低 4 位对应 PA3–PA0）：

```
0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08, 0x09
```

对应通电相为：A → AB → B → BC → C → CD → D → DA → A …

每两个相邻状态只切换一相绕组，相比四拍整步驱动，
半步八拍方式步距更小、运行更平稳、振动更小。

### GPIO 输出

`StepMotor_Init()` 把 PA0–PA3 配置为 50 MHz 推挽输出，初始拉低：

```c
GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
```

### 步进控制

`StepMotor_Rotate(int32_t steps)` 是核心控制函数：

- 参数为正：正转 `steps` 步；为负：反转 `|steps|` 步
- 通过 `static uint8_t phase` 在多次调用间保持当前所处的相位，
  避免每次启动从同一相重新开始而产生抖动
- 每一步通过 `(phase + dir) & 0x07` 在 0–7 之间循环，方向由 `dir` 决定正反转
- 关键写入只用一次寄存器读改写，保证四个引脚同步翻转：

```c
MOTOR_PORT->ODR = (MOTOR_PORT->ODR & ~MOTOR_PINS) | StepSeq[phase];
```

- 每步之间调用 `Delay_us(1500)`，即每 1.5 ms 走一拍。
  转速换算：`1 / (4096 × 1.5 ms) ≈ 0.16 r/s`，约 9.8 RPM，处于 28BYJ-48 推荐工作范围内
- 旋转结束后 `GPIO_ResetBits` 把四相全部断电，避免长时间通电发热与电流损耗

### 主程序流程

`main.c` 演示了基本的使用方式：

```c
StepMotor_Init();
StepMotor_Rotate(STEPS_PER_REV);        // 正转一整圈
Delay_ms(500);                          // 暂停 0.5 s
StepMotor_Rotate(-STEPS_PER_REV / 2);   // 反转半圈
while (1) { }
```

## 工程结构

```
.
├── Libraries/        STM32F10x 标准外设库与 CMSIS
├── Project/          Keil MDK 工程文件（28BYJ48_demo.uvprojx）
├── User/
│   ├── main.c        主程序入口
│   ├── stm32f10x_conf.h / stm32f10x_it.{c,h}
│   └── stepmotor/
│       ├── stepmotor.h   接口与步数常量
│       └── stepmotor.c   八拍时序与 GPIO 控制
├── Output/           编译输出
└── Doc/
```

## 编译与下载

1. 用 Keil MDK-ARM 打开 `Project/28BYJ48_demo.uvprojx`
2. 选择 STM32F103C8 目标并编译
3. 通过 ST-Link/J-Link 下载到 STM32F103C8T6 最小系统板
4. 接好 ULN2003 与 5V 电源后上电，电机即按 `main.c` 中的序列动作

## 调整与扩展

- **改变转速**：修改 `stepmotor.c` 中 `Delay_us(1500)` 的延时值，
  减小延时可提速，但低于 1 ms 时 28BYJ-48 容易失步
- **改用整步四拍**：把 `StepSeq` 改为 `0x01, 0x02, 0x04, 0x08` 并将相位掩码改为 `0x03`，
  转一圈步数变为 2048
- **更精确的延时**：当前 `Delay_us` 是基于 `__NOP` 的软件延时，受编译器优化影响，
  对时间精度要求高时建议改用 SysTick 或 TIMx 定时器
- **非阻塞驱动**：`StepMotor_Rotate` 是阻塞执行的，
  若需要在转动同时处理其他任务，可把单步切换放进定时器中断中调度
