/**
 * @file hr_estimator.c
 * @brief 极简峰间期心率估计（演示用，强光/运动下误差大）
 */
#include "hr_estimator.h"

#define HR_MIN_BPM  45u
#define HR_MAX_BPM  200u

static int32_t abs32(int32_t v)
{
    return v < 0 ? -v : v;
}

static int32_t median3(int32_t a, int32_t b, int32_t c)
{
    if (a > b) {
        int32_t t = a;
        a = b;
        b = t;
    }
    if (b > c) {
        int32_t t = b;
        b = c;
        c = t;
    }
    if (a > b) {
        return a;
    }
    return b;
}

uint8_t hr_estimate_bpm_from_red(const uint32_t *red18, size_t count, unsigned sample_rate_hz)
{
    if (red18 == NULL || count < 64u || sample_rate_hz < 20u) {
        return 0;
    }

    enum { NMAX = 512 };
    if (count > NMAX) {
        count = NMAX;
    }

    int32_t buf[NMAX];
    size_t i;
    for (i = 0; i < count; i++) {
        buf[i] = (int32_t)(red18[i] >> 6);
    }

    for (i = 1; i + 1 < count; i++) {
        buf[i] = median3(buf[i - 1], buf[i], buf[i + 1]);
    }

    /* 一阶差分近似高通 */
    for (i = count - 1; i > 0; i--) {
        buf[i] = buf[i] - buf[i - 1];
    }
    buf[0] = 0;

    int32_t margin = 80;
    size_t last_peak = 0;
    uint32_t peak_intervals_sum = 0;
    unsigned peak_count = 0;

    for (i = 2; i + 2 < count; i++) {
        int32_t v = abs32(buf[i]);
        if (v > margin && v > abs32(buf[i - 1]) && v > abs32(buf[i + 1])) {
            if (last_peak > 0u) {
                size_t dist = i - last_peak;
                unsigned min_dist = (unsigned)(sample_rate_hz * 60u / HR_MAX_BPM);
                unsigned max_dist = (unsigned)(sample_rate_hz * 60u / HR_MIN_BPM);
                if (dist >= min_dist && dist <= max_dist) {
                    peak_intervals_sum += (uint32_t)dist;
                    peak_count++;
                }
            }
            last_peak = i;
        }
    }

    if (peak_count < 2u) {
        return 0;
    }

    uint32_t avg_dist = peak_intervals_sum / peak_count;
    if (avg_dist == 0u) {
        return 0;
    }
    unsigned bpm = (unsigned)((sample_rate_hz * 60u + avg_dist / 2u) / avg_dist);
    if (bpm < HR_MIN_BPM || bpm > HR_MAX_BPM) {
        return 0;
    }
    return (uint8_t)bpm;
}
