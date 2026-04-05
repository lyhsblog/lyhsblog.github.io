# nRF52 + LIS3DH + MAX30102 示例工程（C）

## 数据流（当前 `main.c`）

1. **LIS3DH INT1（GPIOTE 中断）**  
   只做 **`lis3dh_clear_int1()`**，**不在 ISR 里读传感器**（避免 I²C 拉长中断）。

2. **RTC TICK（约 25 ms）**  
   主循环 **`lis3dh_read_accel`** → **`step_counter_*`** 做简易峰检测；**判到一步** 再 **`g_steps_ram++`**（关中断保护）。

3. **RTC COMPARE0（每 60 s）**  
   读走并清零 **`g_steps_ram`** → **`persist_steps_minute()`**（stub）→ 心率：本分钟 **>100 步** 或 **连续低活动满 5 分钟** → 可选测心率 → **`arm(60)`**。

## 文件说明

| 文件 | 作用 |
|------|------|
| `step_counter.c/h` | 加速度域简易计步（供主线调用） |
| `rtc_hr_schedule.c/h` | TICK + COMPARE0 |
| `main.c` | 上述策略 |

其余：`i2c_bus`、`lis3dh`、`max30102`、`hr_estimator`、`board_pins`。

## `sdk_config.h`

需 **RTC2_TICK**（`nrf_drv_rtc_tick_enable`）、TWI、GPIOTE、LFCLK。

## 局限

- `step_counter` 与 `g_steps_ram` 为演示级；量产需标定阈值与佩戴轴向。  
- 心率窗口内仍 `nrf_delay_ms`。
