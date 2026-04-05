/**
 * @file main.c
 * @brief nRF52 + LIS3DH（计步 + 运动 INT1）+ MAX30102（心率 FIFO + 简易 BPM）
 *
 * 依赖：nRF5 SDK 17.x（或 16.x）+ 已启用 TWI0、GPIOTE、CLOCKS。
 * 将本目录 .c 加入 Keil/Ses/CMake 工程，并包含 nrf52840 的 sdk_config.h。
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"

#include "board_pins.h"
#include "i2c_bus.h"
#include "lis3dh.h"
#include "max30102.h"
#include "step_counter.h"
#include "hr_estimator.h"

/* 与 max30102.c 中 REG_SPO2_CONFIG=0x27 对应的采样率须一致；常见为 100 Hz，请以手册为准 */
#define MAX30102_SAMPLE_RATE_HZ  100u

#define HR_MEASURE_WINDOW_MS     4000u
#define HR_POLL_INTERVAL_MS      25u

static volatile bool g_motion_int;

static void lis3dh_int1_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    (void)pin;
    (void)action;
    lis3dh_clear_int1();
    g_motion_int = true;
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

static uint8_t run_one_hr_measurement(void)
{
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

int main(void)
{
    ret_code_t err = i2c_bus_init();
    if (err != NRF_SUCCESS) {
        for (;;) {
            __WFE();
        }
    }

    uint8_t who = 0;
    (void)lis3dh_read_who_am_i(&who);
    (void)lis3dh_init_motion_interrupt();
    gpiote_init_lis3dh();

    uint8_t part = 0;
    (void)max30102_read_part_id(&part);
    (void)max30102_init();

    step_counter_init();

    uint32_t ms_since_hr = 0;
    uint8_t last_bpm = 0;

    for (;;) {
        int16_t ax, ay, az;
        err = lis3dh_read_accel(&ax, &ay, &az);
        if (err == NRF_SUCCESS) {
            (void)step_counter_on_accel_sample(ax, ay, az);
        }

        bool motion = g_motion_int;
        if (motion) {
            g_motion_int = false;
        }

        /* 静止兜底：约每 10 s 测一次心率（演示）；量产请改 RTC / app_timer */
        ms_since_hr += 25u;
        if (motion || ms_since_hr >= 10000u) {
            ms_since_hr = 0;
            last_bpm = run_one_hr_measurement();
            (void)last_bpm;
        }

        nrf_delay_ms(25u);
    }
}
