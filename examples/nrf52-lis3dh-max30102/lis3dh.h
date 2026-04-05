#ifndef LIS3DH_H
#define LIS3DH_H

#include <stdint.h>
#include "sdk_errors.h"

#define LIS3DH_I2C_ADDR_7B  0x18u

ret_code_t lis3dh_init(void);
ret_code_t lis3dh_read_who_am_i(uint8_t *who);
/** ±2 g，约 25 Hz 低功耗，用于计步轮询 */
ret_code_t lis3dh_read_accel(int16_t *x, int16_t *y, int16_t *z);

/** 惯性中断到 INT1（运动唤醒）；需配合 board_pins + GPIOTE */
ret_code_t lis3dh_init_motion_interrupt(void);
void lis3dh_clear_int1(void);

#endif
