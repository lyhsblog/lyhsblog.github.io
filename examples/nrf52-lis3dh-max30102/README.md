# nRF52 + LIS3DH + MAX30102 示例工程（C）

本目录提供可在 **Nordic nRF5 SDK** 工程中集成的 **完整 C 源码**（非 Docusaurus 页面）：I²C 主机、LIS3DH 读加速度与运动中断、MAX30102 心率模式与 FIFO、**简易计步**与 **简易 BPM 估计**。

## 文件说明

| 文件 | 作用 |
|------|------|
| `board_pins.h` | SCL/SDA、LIS3DH INT1 引脚（**务必按原理图修改**） |
| `i2c_bus.c/h` | `nrf_drv_twi` TWI0 封装 |
| `lis3dh.c/h` | WHO_AM_I、读加速度、INT1 惯性中断初始化 |
| `max30102.c/h` | 软复位、Shutdown、HR 模式、FIFO 读出与缓冲收集 |
| `step_counter.c/h` | 幅值 + 高通近似的演示计步（需标定阈值） |
| `hr_estimator.c/h` | 对红灯波形做中值/差分/峰间期的演示 BPM（非医疗级） |
| `main.c` | 约 25 ms 轮询加速度计计步；LIS3DH INT1 或每 10 s 触发一次心率测量 |

## 集成步骤（概要）

1. 新建或打开 **nRF5 SDK** 的 `blank` / `peripheral` 工程（`nrf52840_xxaa` 等）。
2. 将本目录下所有 `.c` 加入工程；头文件路径包含本目录。
3. 在 `sdk_config.h` 中启用：`TWI_ENABLED`、`TWI0_ENABLED`、`TWI0_USE_EASY_DMA`、`GPIOTE_ENABLED`、`NRFX_CLOCK_ENABLED` 等（与 TWI/GPIOTE 示例一致）。
4. 修改 `board_pins.h` 中的 **I²C 与 INT1** 引脚。
5. 确认 LIS3DH **7 位地址**（`0x18` / `0x19`）与 MAX30102 **`0x57`** 与硬件一致。
6. **心率采样率**：`main.c` 中 `MAX30102_SAMPLE_RATE_HZ` 须与 `max30102.c` 里 `REG_SPO2_CONFIG` 实际配置一致（当前按 **100 Hz** 示意，请以 Maxim 手册核对 `0x27` 对应表）。

## 局限说明

- **计步 / BPM 算法**为演示级，量产需大量标定与滤波（运动伪影、肤色、透光等）。
- 未集成 **SoftDevice**、**低功耗睡眠**、**RTC 状态机**；`main` 为阻塞延时循环，便于先调通传感器。
- 未包含 `sdk_config.h` / 启动文件；需在 SDK 工程中由模板提供。

## 与文档站的关系

手环侧栏中的设计说明见站点 `docs/手环/`；本目录为可拷贝进固件仓库的 **参考实现**。
