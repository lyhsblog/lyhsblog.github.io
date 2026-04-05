/**
 * @file main.c
 * @brief 省电 + 心率：非阻塞状态机（RTC TICK 推进），无 nrf_delay_ms 测量窗
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "nrf.h"
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

#define RTC_MINUTE_PERIOD_SEC    60u
#define STEP_THRESHOLD_HIGH      100u
#define LOW_ACTIVITY_MINUTES     5u

#define MOTION_BURST_SEC         12u
#define MOTION_BURST_TICKS       ((uint32_t)MOTION_BURST_SEC * RTC_TICKS_PER_SEC)

static volatile uint32_t g_steps_ram;
static volatile bool g_motion_pending;

/* ---------- 心率：按 RTC TICK 推进（每 tick 约 RTC_MS_PER_TICK ms） ---------- */

typedef enum {
    HR_SM_IDLE = 0,
    HR_SM_START_PPG,
    HR_SM_COLLECT,
    HR_SM_SHUTDOWN
} hr_sm_state_t;

static hr_sm_state_t s_hr_sm;
static uint32_t s_hr_red_buf[512];
static size_t s_hr_total_samples;
static uint16_t s_hr_collect_ticks;
static uint8_t s_hr_last_bpm;

static bool hr_sm_is_busy(void)
{
    return s_hr_sm != HR_SM_IDLE;
}

static void hr_measurement_request_start(void)
{
    rtc_hr_schedule_discard_pending();
    rtc_schedule_step_ticks_flush();

    s_hr_total_samples = 0;
    s_hr_collect_ticks = 0;
    s_hr_sm = HR_SM_START_PPG;
    rtc_schedule_step_ticks_set(true);
}

static void hr_sm_on_tick(void)
{
    ret_code_t err;

    switch (s_hr_sm) {
    case HR_SM_IDLE:
        break;

    case HR_SM_START_PPG:
        err = max30102_mode_hr_run();
        if (err != NRF_SUCCESS) {
            max30102_shutdown();
            s_hr_sm = HR_SM_IDLE;
            rtc_schedule_step_ticks_set(false);
            rtc_schedule_step_ticks_flush();
            return;
        }
        s_hr_sm = HR_SM_COLLECT;
        break;

    case HR_SM_COLLECT: {
        size_t chunk = 0;
        err = max30102_collect_fifo_red(
            s_hr_red_buf + s_hr_total_samples,
            (sizeof(s_hr_red_buf) / sizeof(s_hr_red_buf[0])) - s_hr_total_samples,
            &chunk);
        if (err != NRF_SUCCESS) {
            s_hr_sm = HR_SM_SHUTDOWN;
            break;
        }
        s_hr_total_samples += chunk;
        s_hr_collect_ticks++;

        uint32_t ms = (uint32_t)s_hr_collect_ticks * (uint32_t)RTC_MS_PER_TICK;
        if (ms >= HR_MEASURE_WINDOW_MS ||
            s_hr_total_samples >= (sizeof(s_hr_red_buf) / sizeof(s_hr_red_buf[0]))) {
            s_hr_sm = HR_SM_SHUTDOWN;
        }
        break;
    }

    case HR_SM_SHUTDOWN:
        max30102_shutdown();
        if (s_hr_total_samples < 64u) {
            s_hr_last_bpm = 0;
        } else {
            s_hr_last_bpm = hr_estimate_bpm_from_red(
                s_hr_red_buf, s_hr_total_samples, MAX30102_SAMPLE_RATE_HZ);
        }
        (void)s_hr_last_bpm;

        s_hr_sm = HR_SM_IDLE;
        rtc_schedule_step_ticks_set(false);
        rtc_schedule_step_ticks_flush();
        break;
    }
}

/* -------------------------------------------------------------------------- */

static void lis3dh_int1_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    (void)pin;
    (void)action;
    lis3dh_clear_int1();
    g_motion_pending = true;
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
}

static void step_burst_stop(void)
{
    rtc_schedule_step_ticks_set(false);
    rtc_schedule_step_ticks_flush();
}

static void on_minute_tick(uint32_t *low_streak)
{
    if (hr_sm_is_busy()) {
        /* 不应出现：分钟周期应远大于心率窗口；若出现则跳过本次分钟逻辑 */
        rtc_hr_schedule_arm_seconds(RTC_MINUTE_PERIOD_SEC);
        return;
    }

    step_burst_stop();

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

    if (need_hr) {
        hr_measurement_request_start();
    }

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
    s_hr_sm = HR_SM_IDLE;

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
    uint32_t burst_start = 0;
    bool burst_active = false;

    for (;;) {
        if (g_motion_pending && !hr_sm_is_busy()) {
            __disable_irq();
            g_motion_pending = false;
            __enable_irq();

            burst_start = rtc_schedule_counter_get();
            burst_active = true;
            rtc_schedule_step_ticks_set(true);
            rtc_schedule_step_ticks_flush();
        }

        uint32_t n = rtc_schedule_drain_step_ticks();
        while (n > 0u) {
            n--;
            if (hr_sm_is_busy()) {
                hr_sm_on_tick();
            } else if (burst_active) {
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
        }

        if (burst_active && !hr_sm_is_busy()) {
            uint32_t now = rtc_schedule_counter_get();
            uint32_t elapsed = (now - burst_start) & 0x00FFFFFFu;
            if (elapsed >= MOTION_BURST_TICKS) {
                burst_active = false;
                step_burst_stop();
            }
        }

        if (rtc_hr_schedule_consume_due()) {
            on_minute_tick(&low_activity_streak);
            burst_active = false;
        }

        __WFI();
    }
}
