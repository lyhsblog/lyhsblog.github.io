# nRF52 + LIS3DH + MAX30102 示例工程（C）

本目录提供可在 **Nordic nRF5 SDK** 工程中集成的 **C 源码**：I²C、LIS3DH 运动中断、MAX30102 心率、**RTC 每分钟任务**（RAM 步数落盘 + 条件心率）。

## 业务逻辑（当前 `main.c`）

1. **计步（RAM）**：LIS3DH **INT1 运动中断**每来一次，`g_steps_ram++`（示意：1 次事件计 1；量产可改为多脉冲或读 FIFO）。
2. **RTC（每 60 s）**：**COMPARE0** 唤醒 → 关中断读出 `g_steps_ram` → **`persist_steps_minute()`**（当前为 **stub**，请接 Flash）→ **RAM 清零**。
3. **心率**：
   - 若 **本分钟步数 > `STEP_THRESHOLD_HIGH`（默认 100）** → **本分钟测心率**，并清零「低活动连续分钟」计数；
   - 否则 **低活动分钟计数 +1**；当 **连续 ≥ `LOW_ACTIVITY_MINUTES`（默认 5，可改为 4）** → **测心率** 并清零计数。

主循环仅 **`__WFI()`**，依赖 **RTC 分钟中断**（及测心率期间的 `nrf_delay_ms`）。

## 文件说明

| 文件 | 作用 |
|------|------|
| `board_pins.h` | SCL/SDA、LIS3DH INT1 |
| `i2c_bus.c/h` | TWI0 |
| `lis3dh.c/h` | 运动中断配置、`lis3dh_clear_int1` |
| `max30102.c/h` | MAX30102 FIFO / HR 模式 |
| `hr_estimator.c/h` | 简易 BPM（演示） |
| `rtc_hr_schedule.c/h` | LFCLK + RTC2，**仅 COMPARE0**（无 TICK） |
| `main.c` | 上述策略 |
| `step_counter.c/h` | **未接入当前 main**；可作「加速度域计步」参考 |

## 集成步骤（概要）

1. 将需要的 `.c` 加入工程（若不用 `step_counter`，可不加入 `step_counter.c`）。
2. `sdk_config.h`：`TWI0`、`GPIOTE`、`NRFX_CLOCK`、`RTC2` 等；**无需** `RTC2_TICK`。
3. 修改 `board_pins.h`；实现 **`persist_steps_minute()`** 内真实写盘。
4. 调 **`STEP_THRESHOLD_HIGH`、`LOW_ACTIVITY_MINUTES`**；心率采样率见 `MAX30102_SAMPLE_RATE_HZ` 与 `max30102.c` 中 `REG_SPO2_CONFIG`。

## 局限说明

- 中断内仅 `g_steps_ram++`，与真实「步」的关系需标定；高活动分钟内心率策略为「到分钟边界再测」，非秒级实时。
- 未集成 SoftDevice、完整 PM；心率窗口内仍 `nrf_delay_ms` 轮询 FIFO。

## 与文档站的关系

设计说明见站点 `docs/手环/`。
