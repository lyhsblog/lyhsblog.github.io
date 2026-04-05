/**
 * @file step_counter.c
 * @brief 基于加速度幅值的极简计步（演示用，非医疗级）
 */
#include "step_counter.h"

/* ±2g、12 位有效时 1 LSB ≈ 1 mg；阈值需实测 */
#define STEP_THRESHOLD_RAW   1200
#define STEP_MIN_INTERVAL_MS 280

static uint32_t s_steps;
static int32_t s_lp; /* 一阶低通状态（幅值） */
static uint32_t s_last_step_ms;
static uint32_t s_tick_ms;

void step_counter_init(void)
{
    s_steps = 0;
    s_lp = 0;
    s_last_step_ms = 0;
    s_tick_ms = 0;
}

void step_counter_notify_time_ms(uint32_t dt_ms)
{
    s_tick_ms += dt_ms;
}

uint32_t step_counter_get(void)
{
    return s_steps;
}

static int32_t abs_i32(int32_t v)
{
    return v < 0 ? -v : v;
}

bool step_counter_on_accel_sample(int16_t x, int16_t y, int16_t z)
{
    int32_t ax = x;
    int32_t ay = y;
    int32_t az = z;
    int32_t mag = abs_i32(ax) + abs_i32(ay) + abs_i32(az);

    /* 低通跟踪“准静态”重力 + 姿态，高通近似 = mag - lp */
    int32_t alpha = 8; /* /256 */
    s_lp += (mag - s_lp) * alpha / 256;
    int32_t hp = mag - s_lp;

    bool stepped = false;
    if (hp > STEP_THRESHOLD_RAW && (s_tick_ms - s_last_step_ms) > STEP_MIN_INTERVAL_MS) {
        s_last_step_ms = s_tick_ms;
        s_steps++;
        stepped = true;
    }
    return stepped;
}
