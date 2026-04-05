---
sidebar_position: 4
---

# LIS3DH 与 MAX30102 心率整合样例

## 硬件说明（必读）

**LIS3DH 是三轴加速度计，不能直接测量心率。** 心率由 **MAX30102**（红/红外 LED + 光电接收 + AFE，I²C）完成光学采集；**BPM 需由固件算法**（峰检测、自相关、滤波等）从 FIFO 原始波形算出，本样例只给出 **寄存器配置与读出骨架**，算法可接入 Maxim 应用笔记、SparkFun/Arduino 参考实现或自研库。

本样例中：

- **LIS3DH**：运动检测、静止判断、**INT1 中断**触发「运动开始」等状态切换。
- **MAX30102**：测量窗口内开启 **HR 模式（仅红灯）**、配置采样与 LED 电流、结束时 **关断（Shutdown）** 以省电；窗口末尾从 **FIFO** 读回原始数据并调用 `max30102_estimate_hr_bpm()`（示意，默认返回 0，需你补算法）。

**I²C**：LIS3DH 与 MAX30102 可共用总线；**7 位地址**常见为 LIS3DH `0x18`/`0x19`（视 SDO）、MAX30102 **`0x57`**（以原理图为准）。**LIS3DH INT1** 接 nRF GPIO（GPIOTE）；MAX30102 的 **INT** 可接 GPIO 做「FIFO 将满」唤醒，本样例未依赖该脚，采用轮询/窗末一次性读 FIFO。

**供电**：MAX30102 LED 电流较大，注意 LDO 能力与布线；测量间隙务必 **Shutdown** 或关模式，与《心率传感器低功耗设计方案》一致。

---

## 与《心率传感器低功耗设计方案》的对应关系

| 文档策略 | 本样例实现要点 |
|----------|----------------|
| RTC 兜底（静止 3～5 分钟、测 5 s） | RTC **CC1** 周期触发 `flag_idle_hr`，状态机进入短时测量 |
| 运动开始（加速度计中断、连续 30 s） | LIS3DH **INT1** 置 `flag_motion`，`HR_SM_MOTION_BURST` 内连续开窗 |
| 持续运动（约每 1 分钟、8～10 s） | 在 `HR_SM_ACTIVE` 下由 RTC 秒计数或 CC 调度测量窗口 |
| 运动结束追加一次 | 静止超时后转入 `HR_SM_POST_MOTION` 测 5～8 s |
| Flash 批量写入 | 保留原 **CC0 每分钟** 触发 `write_flash_batch()` 思路 |

参数（阈值、时长、RTC  tick）均为**示意**，量产前需结合结构、佩戴与电流实测标定。

---

## 模块结构

```
LIS3DH(INT1) ──► 运动/唤醒标志
RTC CC0      ──► 每分钟 Flash 落盘
RTC CC1      ──► 静止心率兜底周期（如 3 min）
状态机       ──► 控制 MAX30102 测量窗口时长与频率
MAX30102     ──► FIFO 原始波形 → 算法得 BPM → push_ram → Flash
```

---

## 整合源码（nRF52840 + Nordic `nrf_drv_*` 风格）

以下将 **LIS3DH 寄存器级配置**、**MAX30102 基础驱动与 FIFO 读**、**心率状态机**、**原 RAM 队列 + fstorage** 合在一处。  
**TWI 底层**请按工程替换为 `nrf_drv_twi` / `nrfx_twi_m` 等；**SoftDevice + `nrf_fstorage_process`** 仍需按官方示例补全。  
**MAX30102 寄存器值**（采样率、脉宽、LED 电流、FIFO 水深）请按结构光路与功耗在实机上标定，下表为常见示意值。

```c
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nrf_drv_rtc.h"
#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

/* -------------------------------------------------------------------------- */
/* 引脚与 I2C（按硬件修改）                                                     */
/* -------------------------------------------------------------------------- */

#define LIS3DH_I2C_ADDR        0x18u   /* SDO 接 GND 时常为 0x18，接 VDD 为 0x19 */
#define LIS3DH_INT1_PIN        16      /* LIS3DH INT1 → nRF GPIO */

#define MAX30102_I2C_ADDR      0x57u   /* 常见 7 位地址；以模块原理图为准 */

#define RTC_TICK_HZ            1u      /* 假设预分频后 RTC 计数 1 Hz，需与 init 一致 */
#define RTC_CC0_FLASH_SEC      60u     /* 每分钟 Flash */
#define RTC_CC1_IDLE_HR_SEC    180u    /* 静止兜底：每 3 分钟（可改 300 = 5 分钟） */

#define MAX_RAM_QUEUE          128u
#define FLASH_START_ADDR       0x3F0000u
#define FLASH_PAGE_SIZE        4096u

/* -------------------------------------------------------------------------- */
/* LIS3DH 寄存器（节选）                                                       */
/* -------------------------------------------------------------------------- */

#define LIS3DH_REG_WHO_AM_I       0x0Fu
#define LIS3DH_REG_CTRL_REG1      0x20u
#define LIS3DH_REG_CTRL_REG2      0x21u
#define LIS3DH_REG_CTRL_REG3      0x22u
#define LIS3DH_REG_CTRL_REG4      0x23u
#define LIS3DH_REG_REFERENCE      0x26u
#define LIS3DH_REG_INT1_CFG       0x30u
#define LIS3DH_REG_INT1_SRC       0x31u   /* 读清中断 */
#define LIS3DH_REG_INT1_THS       0x32u
#define LIS3DH_REG_INT1_DURATION  0x33u

#define LIS3DH_SUB_READ_MULT      0x80u   /* SPI/I2C 子地址自动增量 */

/* -------------------------------------------------------------------------- */
/* 数据与 Flash（与原样例一致，可按需扩展 activity 等字段）                    */
/* -------------------------------------------------------------------------- */

typedef struct {
    uint32_t timestamp;
    uint16_t steps;
    uint8_t  heart_rate;
} data_t;

static data_t ram_queue[MAX_RAM_QUEUE];
static volatile uint8_t ram_head;
static volatile uint8_t ram_tail;
static uint32_t flash_addr = FLASH_START_ADDR;

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    .evt_handler = NULL,
    .start_addr  = FLASH_START_ADDR,
    .end_addr    = 0x3FFFFF,
};

/* -------------------------------------------------------------------------- */
/* TWI 桩：请替换为真实 TWI 写/读                                             */
/* -------------------------------------------------------------------------- */

/* 返回 0 表示成功；请接 TWI 驱动后按需改为 ret_code_t */
static int twi_write(uint8_t slave_addr, uint8_t const *data, uint8_t length);
static int twi_read(uint8_t slave_addr, uint8_t reg, uint8_t *data, uint8_t length);

static void lis3dh_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    (void)twi_write(LIS3DH_I2C_ADDR, buf, 2);
}

static void lis3dh_read_regs(uint8_t reg, uint8_t *dst, uint8_t len)
{
    uint8_t r = (uint8_t)(reg | LIS3DH_SUB_READ_MULT);
    (void)twi_read(LIS3DH_I2C_ADDR, r, dst, len);
}

static uint8_t lis3dh_clear_int1_src(void)
{
    uint8_t src = 0;
    lis3dh_read_regs(LIS3DH_REG_INT1_SRC, &src, 1);
    return src;
}

/**
 * LIS3DH：惯性中断到 INT1（阈值 + 持续时间，需按 mg 与 ODR 标定）。
 * 以下为示意值：INT1_THS、DURATION、CTRL_REG1 ODR 务必在实机上调整。
 */
static void lis3dh_init_motion_int1(void)
{
    uint8_t who = 0;
    lis3dh_read_regs(LIS3DH_REG_WHO_AM_I, &who, 1);
    /* 期望 who == 0x33，开发阶段可断言或打日志 */

    lis3dh_write_reg(LIS3DH_REG_CTRL_REG1, 0x57); /* 约 50 Hz + LP + XYZ，示例 */
    lis3dh_write_reg(LIS3DH_REG_CTRL_REG4, 0x08); /* BDU=1，±2g 等按需要改 */
    lis3dh_write_reg(LIS3DH_REG_REFERENCE, 0x00);

    /* 高通用于中断路径（与 AN3308 类似，具体以 ST 手册为准） */
    lis3dh_write_reg(LIS3DH_REG_CTRL_REG2, 0x09);

    lis3dh_write_reg(LIS3DH_REG_INT1_THS, 0x10);      /* 阈值：示意 */
    lis3dh_write_reg(LIS3DH_REG_INT1_DURATION, 0x03); /* 持续时间：示意 */
    lis3dh_write_reg(LIS3DH_REG_INT1_CFG, 0x2A);      /* XH|YH|ZH，OR 组合：示意 */

    /* CTRL_REG3：惯性中断 1 映射到 INT1 引脚 */
    lis3dh_write_reg(LIS3DH_REG_CTRL_REG3, 0x40);     /* I1_IA1 */

    (void)lis3dh_clear_int1_src();
}

/* -------------------------------------------------------------------------- */
/* MAX30102 寄存器（节选，见 Maxim 数据手册）                                    */
/* -------------------------------------------------------------------------- */

#define MAX30102_REG_INT_STATUS1   0x00u
#define MAX30102_REG_INT_STATUS2   0x01u
#define MAX30102_REG_INT_EN1       0x02u
#define MAX30102_REG_INT_EN2       0x03u
#define MAX30102_REG_FIFO_WR_PTR   0x04u
#define MAX30102_REG_OVF_COUNTER   0x05u
#define MAX30102_REG_FIFO_RD_PTR   0x06u
#define MAX30102_REG_FIFO_DATA     0x07u
#define MAX30102_REG_FIFO_CONFIG   0x08u
#define MAX30102_REG_MODE_CONFIG   0x09u
#define MAX30102_REG_SPO2_CONFIG   0x0Au
#define MAX30102_REG_LED1_PA       0x0Cu  /* 红灯电流 */
#define MAX30102_REG_LED2_PA       0x0Du  /* 红外电流 */
#define MAX30102_REG_PART_ID       0xFFu  /* MAX30102 常为 0x15 */

#define MAX30102_MODE_SHDN         0x80u
#define MAX30102_MODE_RESET        0x40u
#define MAX30102_MODE_HR_ONLY      0x02u  /* 仅红灯，心率模式 */

static uint8_t s_hr_bpm_smoothed; /* 由算法更新；此处为窗末估算结果 */

static void max30102_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    (void)twi_write(MAX30102_I2C_ADDR, buf, 2);
}

static void max30102_read_reg(uint8_t reg, uint8_t *dst, uint8_t len)
{
    (void)twi_read(MAX30102_I2C_ADDR, reg, dst, len);
}

static uint8_t max30102_fifo_sample_count(void)
{
    uint8_t w = 0, r = 0;
    max30102_read_reg(MAX30102_REG_FIFO_WR_PTR, &w, 1);
    max30102_read_reg(MAX30102_REG_FIFO_RD_PTR, &r, 1);
    w &= 0x1Fu;
    r &= 0x1Fu;
    /* 32 级 FIFO 环：未读样本数（满环时手册另有标志，量产需结合 OVF） */
    return (uint8_t)((w - r + 32u) & 0x1Fu);
}

static void max30102_clear_int_status(void)
{
    uint8_t s1 = 0, s2 = 0;
    max30102_read_reg(MAX30102_REG_INT_STATUS1, &s1, 1);
    max30102_read_reg(MAX30102_REG_INT_STATUS2, &s2, 1);
    (void)s1;
    (void)s2;
}

/**
 * 上电默认配置：软复位后设 FIFO、HR 模式、采样与红灯电流。
 * REG_SPO2_CONFIG / LED 电流务必按佩戴与结构调参。
 */
static void max30102_init(void)
{
    max30102_write_reg(MAX30102_REG_MODE_CONFIG, MAX30102_MODE_RESET);
    nrf_delay_ms(10);
    max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x00u);

    max30102_clear_int_status();
    max30102_write_reg(MAX30102_REG_INT_EN1, 0x00u);
    max30102_write_reg(MAX30102_REG_INT_EN2, 0x00u);

    max30102_write_reg(MAX30102_REG_FIFO_WR_PTR, 0x00u);
    max30102_write_reg(MAX30102_REG_OVF_COUNTER, 0x00u);
    max30102_write_reg(MAX30102_REG_FIFO_RD_PTR, 0x00u);

    /* FIFO：示例为 4 样本平均、允许 rollover、almost-full 阈值示意 */
    max30102_write_reg(MAX30102_REG_FIFO_CONFIG, 0x4Fu);
    /* 采样率 / ADC / 脉宽：示意值 0x27，请按手册与实测替换 */
    max30102_write_reg(MAX30102_REG_SPO2_CONFIG, 0x27u);

    max30102_write_reg(MAX30102_REG_LED1_PA, 0x24u); /* 红灯，约数 mA 级，需标定 */
    max30102_write_reg(MAX30102_REG_LED2_PA, 0x00u); /* HR-only 可关 IR */

    max30102_write_reg(MAX30102_REG_MODE_CONFIG, MAX30102_MODE_SHDN); /* 先关断，测时再开 */

    uint8_t part = 0;
    max30102_read_reg(MAX30102_REG_PART_ID, &part, 1);
    (void)part; /* 期望 0x15，开发阶段可断言 */
}

static void max30102_estimate_hr_from_fifo(void);

/** 测量窗口开始：退出 shutdown、HR 模式、清 FIFO 指针 */
static void max30102_measure_start(void)
{
    max30102_clear_int_status();
    max30102_write_reg(MAX30102_REG_FIFO_WR_PTR, 0x00u);
    max30102_write_reg(MAX30102_REG_OVF_COUNTER, 0x00u);
    max30102_write_reg(MAX30102_REG_FIFO_RD_PTR, 0x00u);
    max30102_write_reg(MAX30102_REG_MODE_CONFIG, MAX30102_MODE_HR_ONLY);
}

/** 测量窗口结束：shutdown 省电；读 FIFO 并估算 BPM（示意） */
static void max30102_measure_stop(void)
{
    max30102_estimate_hr_from_fifo();
    max30102_write_reg(MAX30102_REG_MODE_CONFIG, MAX30102_MODE_SHDN);
}

/**
 * 从 FIFO 读出当前积压样本（HR 模式每样本 3 字节，红灯 18-bit）。
 * 应在窗末或周期调用；此处仅 Drain + 占位算法。
 */
static void max30102_estimate_hr_from_fifo(void)
{
    uint8_t n = max30102_fifo_sample_count();
    uint8_t i;
    uint32_t red;

    for (i = 0; i < n; i++) {
        uint8_t raw[3];
        max30102_read_reg(MAX30102_REG_FIFO_DATA, raw, 3);
        red = ((uint32_t)raw[0] << 16) | ((uint32_t)raw[1] << 8) | (uint32_t)raw[2];
        (void)red;
        /* TODO: 送入滤波 + 峰间期 / 自相关，更新 s_hr_bpm_smoothed */
    }
    /* 无算法时保持 0；接入算法后写入 40～220 合理范围并做指数平滑 */
    if (n < 3u) {
        s_hr_bpm_smoothed = 0;
    }
}

static uint8_t max30102_read_bpm_smoothed(void)
{
    return s_hr_bpm_smoothed;
}

/* 简单计步桩（可用 LIS3DH 数据做计步算法） */
static uint16_t read_steps_estimate(void) { return 0; }

/* -------------------------------------------------------------------------- */
/* RAM 队列 + Flash                                                            */
/* -------------------------------------------------------------------------- */

static void push_ram(data_t *data)
{
    uint8_t next_head = (uint8_t)((ram_head + 1u) % MAX_RAM_QUEUE);
    if (next_head != ram_tail) {
        ram_queue[ram_head] = *data;
        ram_head = next_head;
    }
}

static void write_flash_batch(void)
{
    while (ram_tail != ram_head) {
        data_t *d = &ram_queue[ram_tail];
        if (((flash_addr - FLASH_START_ADDR) % FLASH_PAGE_SIZE) == 0u) {
            (void)nrf_fstorage_erase(&fstorage, flash_addr, 1, NULL);
        }
        (void)nrf_fstorage_write(&fstorage, flash_addr, d, sizeof(data_t), NULL);
        flash_addr += sizeof(data_t);
        ram_tail = (uint8_t)((ram_tail + 1u) % MAX_RAM_QUEUE);
    }
}

/* -------------------------------------------------------------------------- */
/* RTC：CC0 落盘；CC1 静止心率兜底；每秒 tick 供状态机计时                      */
/* -------------------------------------------------------------------------- */

static volatile bool g_flag_second_tick;
static volatile bool g_flag_flash_minute;
static volatile bool g_flag_idle_hr;

static const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(0);

static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
    if (int_type == NRF_DRV_RTC_INT_COMPARE0) {
        g_flag_flash_minute = true;
    } else if (int_type == NRF_DRV_RTC_INT_COMPARE1) {
        g_flag_idle_hr = true;
    } else if (int_type == NRF_DRV_RTC_INT_TICK) {
        g_flag_second_tick = true;
    }
}

static void init_rtc_schedules(void)
{
    nrf_drv_rtc_config_t cfg = NRF_DRV_RTC_DEFAULT_CONFIG;
    (void)nrf_drv_rtc_init(&rtc, &cfg, rtc_handler);

    /* 1 Hz tick：用于测量窗口秒计数 */
    nrf_drv_rtc_tick_enable(&rtc);

    uint32_t t0 = RTC_CC0_FLASH_SEC * RTC_TICK_HZ;
    uint32_t t1 = RTC_CC1_IDLE_HR_SEC * RTC_TICK_HZ;
    nrf_drv_rtc_cc_set(&rtc, 0, t0, true);
    nrf_drv_rtc_cc_set(&rtc, 1, t1, true);

    nrf_drv_rtc_enable(&rtc);
}

/* 读 RTC COUNTER 作简易时间戳（也可用日历） */
static uint32_t rtc_counter_now(void)
{
    return nrf_drv_rtc_counter_get(&rtc);
}

/* -------------------------------------------------------------------------- */
/* 心率状态机（与低功耗设计文档对齐的简化版）                                    */
/* -------------------------------------------------------------------------- */

typedef enum {
    HR_SM_IDLE = 0,
    HR_SM_MOTION_BURST,   /* 运动开始：连续测约 30 s */
    HR_SM_ACTIVE,         /* 持续运动：每 60 s 测 8～10 s */
    HR_SM_POST_MOTION     /* 运动结束：追加 5～8 s */
} hr_sm_state_t;

static hr_sm_state_t s_hr_sm = HR_SM_IDLE;
static uint16_t s_measure_remaining_s;
static uint16_t s_active_interval_s;
static uint16_t s_still_count_s;

#define MOTION_BURST_TOTAL_S    30u
#define ACTIVE_MEASURE_S        9u     /* 8～10 s 取中 */
#define ACTIVE_INTERVAL_S       60u
#define POST_MOTION_MEASURE_S   6u     /* 5～8 s */
#define IDLE_MEASURE_S          5u
#define STILL_SECONDS_TO_END    120u   /* 连续静止判定“运动结束”，示意 */

static void hr_start_window(uint16_t seconds)
{
    s_measure_remaining_s = seconds;
    max30102_measure_start();
}

static void hr_stop_window(void)
{
    max30102_measure_stop();
    s_measure_remaining_s = 0;
}

static void hr_sample_and_push(void)
{
    data_t d;
    d.timestamp = rtc_counter_now();
    d.steps = read_steps_estimate();
    d.heart_rate = max30102_read_bpm_smoothed();
    push_ram(&d);
}

typedef struct {
    bool second_tick;
    bool idle_hr;
    bool flash_minute;
} wake_events_t;

static wake_events_t pull_wake_events(void)
{
    wake_events_t e = { false, false, false };
    if (g_flag_second_tick) {
        g_flag_second_tick = false;
        e.second_tick = true;
    }
    if (g_flag_idle_hr) {
        g_flag_idle_hr = false;
        e.idle_hr = true;
    }
    if (g_flag_flash_minute) {
        g_flag_flash_minute = false;
        e.flash_minute = true;
    }
    return e;
}

static volatile bool g_flag_motion_int;

static void lis3dh_int1_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    (void)pin;
    (void)action;
    (void)lis3dh_clear_int1_src();
    g_flag_motion_int = true;
}

static void init_lis3dh_interrupt(void)
{
    (void)nrf_drv_gpiote_init();
    nrf_drv_gpiote_in_config_t cfg = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    cfg.pull = NRF_GPIO_PIN_PULLDOWN;
    (void)nrf_drv_gpiote_in_init(LIS3DH_INT1_PIN, &cfg, lis3dh_int1_handler);
    nrf_drv_gpiote_in_event_enable(LIS3DH_INT1_PIN, true);
}

static void hr_sm_on_second_tick(void)
{
    if (s_measure_remaining_s > 0u) {
        s_measure_remaining_s--;
        if (s_measure_remaining_s == 0u) {
            hr_sample_and_push();
            hr_stop_window();

            if (s_hr_sm == HR_SM_MOTION_BURST) {
                /* 30 s 连续窗结束：进入持续运动，再等约 1 分钟做第一次周期测量 */
                s_hr_sm = HR_SM_ACTIVE;
                s_active_interval_s = ACTIVE_INTERVAL_S;
                s_still_count_s = 0;
            } else if (s_hr_sm == HR_SM_POST_MOTION) {
                s_hr_sm = HR_SM_IDLE;
            } else if (s_hr_sm == HR_SM_ACTIVE) {
                /* 本轮 8～10 s 测完，启动下一周期倒计时 */
                s_active_interval_s = ACTIVE_INTERVAL_S;
            }
        }
        return;
    }

    if (s_hr_sm == HR_SM_ACTIVE) {
        if (s_active_interval_s > 0u) {
            s_active_interval_s--;
        }
        if (s_active_interval_s == 0u) {
            hr_start_window(ACTIVE_MEASURE_S);
            s_measure_remaining_s = ACTIVE_MEASURE_S;
        }
        /* 仅在「两次测量之间」累加静止秒数；有 LIS3DH INT1 时在 hr_sm_on_motion_int 中清零 */
        s_still_count_s++;
        if (s_still_count_s >= STILL_SECONDS_TO_END) {
            s_hr_sm = HR_SM_POST_MOTION;
            s_active_interval_s = 0;
            hr_start_window(POST_MOTION_MEASURE_S);
            s_measure_remaining_s = POST_MOTION_MEASURE_S;
            s_still_count_s = 0;
        }
    }
}

static void hr_sm_on_motion_int(void)
{
    g_flag_motion_int = false;
    s_still_count_s = 0;

    if (s_hr_sm == HR_SM_IDLE || s_hr_sm == HR_SM_POST_MOTION) {
        s_hr_sm = HR_SM_MOTION_BURST;
        hr_start_window(MOTION_BURST_TOTAL_S);
        s_measure_remaining_s = MOTION_BURST_TOTAL_S;
    }
    /* HR_SM_ACTIVE：已在上方清零 s_still_count_s，避免误判运动结束 */
    /* HR_SM_MOTION_BURST：可选在窗内再次 INT1 时重置 30 s，本样例从简不重置 */
}

static void hr_sm_on_idle_rtc(void)
{
    /* 仅在静止状态做 RTC 兜底，避免与运动窗冲突 */
    if (s_hr_sm != HR_SM_IDLE) {
        return;
    }
    hr_start_window(IDLE_MEASURE_S);
    s_measure_remaining_s = IDLE_MEASURE_S;
}

static void hr_state_machine_poll(wake_events_t e)
{
    /* 运动中断优先处理，避免与秒节拍顺序颠倒 */
    if (g_flag_motion_int) {
        hr_sm_on_motion_int();
    }
    if (e.second_tick) {
        hr_sm_on_second_tick();
    }
    if (e.idle_hr) {
        hr_sm_on_idle_rtc();
    }
}

/* -------------------------------------------------------------------------- */
/* main                                                                        */
/* -------------------------------------------------------------------------- */

int main(void)
{
    /* TWI / 时钟 / SoftDevice 等：按工程初始化 */
    max30102_init();
    lis3dh_init_motion_int1();
    init_lis3dh_interrupt();

    (void)nrf_fstorage_init(&fstorage, &nrf_fstorage_sd, NULL);

    init_rtc_schedules();

    for (;;) {
        __WFE();

        wake_events_t ev = pull_wake_events();

        if (ev.flash_minute) {
            write_flash_batch();
            /* nrf_fstorage_process(&fstorage);  // 真实工程必须周期性调用 */
        }

        hr_state_machine_poll(ev);
    }
}

/* -------------------------------------------------------------------------- */
/* TWI 桩实现占位（避免链接错误，请删除并接驱动）                                */
/* -------------------------------------------------------------------------- */

static int twi_write(uint8_t slave_addr, uint8_t const *data, uint8_t length)
{
    (void)slave_addr;
    (void)data;
    (void)length;
    return 0;
}

static int twi_read(uint8_t slave_addr, uint8_t reg, uint8_t *data, uint8_t length)
{
    (void)slave_addr;
    (void)reg;
    (void)data;
    (void)length;
    return 0;
}
```

---

## 使用与调参提示

1. **LIS3DH**：先读 `WHO_AM_I` 是否为 **0x33**，再调 **INT1_THS / DURATION / CTRL_REG1 ODR**，避免误触发或漏触发。  
2. **MAX30102**：上电读 `PART_ID`（**0x15**）确认通信；重点调 **LED1_PA**、**SPO2_CONFIG**（采样率 / ADC / 脉宽）、**FIFO_CONFIG**。窗内可轮询读 FIFO 或在 **INT** 引脚用「almost full」唤醒，避免漏读溢出。  
3. **心率算法**：`max30102_estimate_hr_from_fifo()` 内需对 **18-bit 红灯**序列做 DC 去除、带通/滑动平均、峰间期或自相关，再与《心率传感器低功耗设计方案》中的 **指数平滑** 结合；无算法时 `s_hr_bpm_smoothed` 可能为 0。  
4. **静止判定**：上例用「运动中长时间无 INT1」近似静止；更稳妥可读 `OUT_*` 与阈值比较，或与计步结果融合。  
5. **RTC**：若实际 tick 不是 1 Hz，需按比例修改 `RTC_CC0_FLASH_SEC`、`RTC_CC1_IDLE_HR_SEC` 及状态机里的「秒」计数。部分 SDK 配置下 **COMPARE 为单次事件**，需在 `rtc_handler` 里对 CC0/CC1 **重新写入下一比较值**（或改用 TICK 累计实现「每分钟 / 每 3 分钟」节拍），否则兜底与落盘只触发一次。  
6. **功耗**：测量窗之间 **MODE_CONFIG bit7 Shutdown** 关断 MAX30102；LIS3DH 可在静止时降低 ODR 或进低功耗（需重配中断策略）。  
7. **fstorage**：Flash 写入仍建议放在 **主线程** 或 **低优先级任务**，并配合 `nrf_fstorage_process`，避免在 RTC 回调里长时间阻塞。

---

## 与站内其它文档的关系

- 数据落盘与队列思路见 [手环程序说明](./手环程序说明.md)。  
- 状态时长与策略依据见 [心率传感器低功耗设计方案](./心率传感器低功耗设计方案.md)。
