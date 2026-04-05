# nRF52 + LIS3DH + MAX30102 示例工程（C）

## 数据流与省电（当前 `main.c`）

1. **LIS3DH INT1**  
   ISR：**`lis3dh_clear_int1()`** + 置 **`g_motion_pending`**（不读传感器）。

2. **静止**  
   **RTC TICK 关闭**，CPU **`__WFI()`** 仅由 **COMPARE0（每分钟）** 或 **GPIOTE** 唤醒，**无 25 ms 周期唤醒**。

3. **运动**  
   主循环见到 **`g_motion_pending`** → **打开 TICK** + 刷新 **`MOTION_BURST_SEC`（默认 12 s）** 窗口起点；窗口内约 **25 ms** 读加速度计步。  
   **再次 INT1 会重置窗口起点**（持续走动会续期）。超时 **关 TICK**。

4. **每分钟 COMPARE0**  
   先 **关计步爆发** → 读走 **`g_steps_ram`** → 落盘 stub → 若需测心率则启动 **非阻塞状态机**（**RTC TICK** 每拍：`START_PPG` → 多拍 **读 FIFO** → **Shutdown** + 算 BPM），**无 `nrf_delay_ms`**。心率结束后 **关 TICK**（除非运动爆发仍应开启，由 `g_motion_pending` 下一圈处理）。

可调 **`MOTION_BURST_SEC`**：越大跟手越好、越费电；越小越省、可能漏步至下次中断。

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
- 心率窗口由 **RTC TICK 状态机** 推进，主循环不阻塞；`MAX30102` 采样率与 `HR_MEASURE_WINDOW_MS` / `RTC_MS_PER_TICK` 需一致理解（约 160 tick ≈ 4 s）。
