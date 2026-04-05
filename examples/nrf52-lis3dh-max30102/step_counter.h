#ifndef STEP_COUNTER_H
#define STEP_COUNTER_H

#include <stdint.h>
#include <stdbool.h>

void step_counter_init(void);
uint32_t step_counter_get(void);

/**
 * 传入 LIS3DH 原始加速度（与 read_accel 一致，有符号 16 位）。
 * 内部用简易高通 + 幅值峰值检测；参数需按佩戴位置标定。
 */
bool step_counter_on_accel_sample(int16_t x, int16_t y, int16_t z);

#endif
