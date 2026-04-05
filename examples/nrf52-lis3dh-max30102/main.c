/**
 * @file main.c
 * @brief LIS3DH 运动 INT 仅清中断；计步在主循环读加速度；RTC 每分钟落盘 + 条件心率
 *
 * - GPIOTE ISR：只做 lis3dh_clear_int1()（不在 ISR 里 I²C 读数据，避免中断过长）。
 * - RTC TICK ≈25 ms：主循环读 lis3dh_read_accel → step_counter → 判到一步则 g_steps_ram++。
 * - RTC COMPARE0 每 60 s：persist + 清零 g_steps_ram + 心率规则。
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
#include "step_counter.h"
#include "rtc_hr_schedule.h"

#define MAX30102_SAMPLE_RATE_HZ  100u
#define HR_MEASURE_WINDOW_MS     4000u
#define HR_POLL_INTERVAL_MS      25u

#define RTC_MINUTE_PERIOD_SEC    60u
#define STEP_THRESHOLD_HIGH      100u
#define LOW_ACTIVITY_MINUTES     5u

/* 本分钟内由「加速度计步算法」累加的步数；每分钟任务读走并清零 */
static volatile uint32_t g_steps_ram;

static void lis3dh_int1_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    (void)pin;
    (void)action;
    /* 只清 LIS3DH 锁存；步数在主线读 OUT_X/Y/Z + step_counter 判定 */
    lis3dh_clear_int1();
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

static void persist_steps_minute(uint32_t steps_this_minute)
{
    (void)steps_this_minute;
    /* TODO: Flash */
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

    step_counter_init();

    err = rtc_hr_schedule_init(RTC_MINUTE_PERIOD_SEC);
    if (err != NRF_SUCCESS) {
        for (;;) {
            __WFE();
        }
    }

    uint32_t low_activity_streak = 0;

    for (;;) {
        uint32_t n = rtc_schedule_drain_step_ticks();
        while (n > 0u) {
            n--;
            int16_t ax, ay, az;
            err = lis3dh_read_accel(&ax, &ay, &az);
            if (err == NRF_SUCCESS) {
                step_counter_notify_time_ms(RTC_MS_PER_TICK);
                if (step_counter_on_accel_sample(ax, ay, az)) {
                    __disable_irq();
                    g_steps_ram++;
                    __enable_irq();
                }
            }
        }

        if (rtc_hr_schedule_consume_due()) {
            on_minute_tick(&low_activity_streak);
        }

        __WFI();
    }
}
