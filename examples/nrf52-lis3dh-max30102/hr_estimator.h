#ifndef HR_ESTIMATOR_H
#define HR_ESTIMATOR_H

#include <stdint.h>
#include <stddef.h>

/**
 * 根据 MAX30102 红灯 18-bit 序列估计 BPM（演示算法）。
 * @param sample_rate_hz 须与 REG_SPO2_CONFIG 实际采样率一致，默认按 100 Hz 示意
 */
uint8_t hr_estimate_bpm_from_red(const uint32_t *red18, size_t count, unsigned sample_rate_hz);

#endif
