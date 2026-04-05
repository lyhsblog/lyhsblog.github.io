/**
 * @file main.c
 * @brief nRF52 + LIS3DH（运动中断 RAM 计步）+ MAX30102（心率）+ RTC 每分钟任务
 *
 * 策略（与用户约定一致）：
 * - 运动检测 INT1 每触发一次 → RAM 中本周期步数累加（示意：1 次中断 +1，阈值需标定）。
 * - RTC COMPARE0 每 60 s → 读出 RAM 步数 → 持久化（stub）→ RAM 清零；
 *   心率：本分钟步数 > STEP_THRESHOLD_HIGH 则本分钟测；否则仅当连续低活动满 LOW_ACTIVITY_MINUTES 分钟才测。
 *
 * 依赖：TWI0、GPIOTE、LFCLK、RTC2（无 TICK）。心率窗口内仍用 nrf_delay_ms。
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"

#include "board_pins.h"
#include "i2c_bus.h"
#include "lis3dh.h"
#include "max30102.h"
#include "hr_estimator.h"
#include "rtc_hr_schedule.h"

#define MAX30102_SAMPLE_RATE_HZ  100u
#define HR_MEASURE_WINDOW_MS     4000u
#define HR_POLL_INTERVAL_MS      25u

/** RTC：整分钟节拍（秒） */
#define RTC_MINUTE_PERIOD_SEC    60u

/** 本分钟 RAM 步数超过此值 → 本分钟直接测心率 */
#define STEP_THRESHOLD_HIGH      100u

/** 否则：连续「步数未超过阈值」的分钟数达到此值才测心率（4～5 分钟，此处用 5） */
#define LOW_ACTIVITY_MINUTES     5u

/* 本统计周期内由运动中断累加的步数（RAM）；每分钟 RTC 任务读走并清零 */
static volatile uint32_t g_steps_ram;

static void lis3dh_int1_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    (void)pin;
    (void)action;
    lis3dh_clear_int1();
    /* 示意：每次运动中断计 1「步」/事件；量产可改为脉冲计数或读 LIS3DH 数据源 */
    g_steps_ram++;
}

static void gpiote_init_lis3dh(void)
{
    ret_code_t err = nrf_drv_gpiote_init();
    if (err != NRF_SUCCESS && err != NRF_ERROR_INVALID_STATE) {
        return;
    }

    nrf_drv_gpiote_in_config_t cfg = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    cfg.pull = NRF_GPIO_PIN_PULLDOWN;
    err = nrf_drv_gpiote_in_init(BOARD_LIS3DH_INT1, &cfg, lis3dh_int1_handler);
    if (err == NRF_SUCCESS) {
        nrf_drv_gpiote_in_event_enable(BOARD_LIS3DH_INT1, true);
    }
}

/** 将上一分钟步数写入 Flash/文件等；此处为 stub */
static void persist_steps_minute(uint32_t steps_this_minute)
{
    (void)steps_this_minute;
    /* TODO: nrf_fstorage 或外部 Flash；避免在 stub 中长时间阻塞 */
}

static uint8_t run_one_hr_measurement(void)
{
    rtc_hr_schedule_discard_pending();

    ret_code_t err = max30102_mode_hr_run();
    if (err != NRF_SUCCESS) {
        max30102_shutdown();
        return 0;
    }

    uint32_t red_buf[512];
    size_t total = 0;

    uint32_t elapsed = 0;
    while (elapsed < HR_MEASURE_WINDOW_MS) {
        nrf_delay_ms(HR_POLL_INTERVAL_MS);
        elapsed += HR_POLL_INTERVAL_MS;

        size_t chunk = 0;
        err = max30102_collect_fifo_red(red_buf + total, sizeof(red_buf) / sizeof(red_buf[0]) - total, &chunk);
        if (err != NRF_SUCCESS) {
            break;
        }
        total += chunk;
        if (total >= sizeof(red_buf) / sizeof(red_buf[0])) {
            break;
        }
    }

    max30102_shutdown();

    if (total < 64u) {
        return 0;
    }
    return hr_estimate_bpm_from_red(red_buf, total, MAX30102_SAMPLE_RATE_HZ);
}

/**
 * 每分钟 RTC 到期时调用：落盘、清零 RAM 步数、按规则决定是否测心率。
 * @param low_streak 连续「步数 ≤ STEP_THRESHOLD_HIGH」的分钟数（由调用方读写）
 */
static void on_minute_tick(uint32_t *low_streak)
{
    uint32_t steps;

    __disable_irq();
    steps = g_steps_ram;
    g_steps_ram = 0;
    __enable_irq();

    persist_steps_minute(steps);

    bool need_hr = false;
    if (steps > STEP_THRESHOLD_HIGH) {
        need_hr = true;
        *low_streak = 0;
    } else {
        (*low_streak)++;
        if (*low_streak >= LOW_ACTIVITY_MINUTES) {
            need_hr = true;
            *low_streak = 0;
        }
    }

    uint8_t bpm = 0;
    if (need_hr) {
        bpm = run_one_hr_measurement();
    }
    (void)bpm;

    rtc_hr_schedule_arm_seconds(RTC_MINUTE_PERIOD_SEC);
}

int main(void)
{
    ret_code_t err = i2c_bus_init();
    if (err != NRF_SUCCESS) {
        for (;;) {
            __WFE();
        }
    }

    uint8_t who = 0;
    uint8_t part = 0;
    (void)lis3dh_read_who_am_i(&who);
    (void)lis3dh_init_motion_interrupt();
    gpiote_init_lis3dh();

    (void)max30102_read_part_id(&part);
    (void)max30102_init();

    err = rtc_hr_schedule_init(RTC_MINUTE_PERIOD_SEC);
    if (err != NRF_SUCCESS) {
        for (;;) {
            __WFE();
        }
    }

    uint32_t low_activity_streak = 0;

    for (;;) {
        if (rtc_hr_schedule_consume_due()) {
            on_minute_tick(&low_activity_streak);
        }
        __WFI();
    }
}
