#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stdint.h>
#include <stddef.h>
#include "sdk_errors.h"

ret_code_t i2c_bus_init(void);
void i2c_bus_uninit(void);

/** 写寄存器：reg + data[0..len-1] */
ret_code_t i2c_write_regs(uint8_t addr7, uint8_t reg, uint8_t const *data, size_t len);

/**
 * 读寄存器：先写 reg，再读 len 字节。
 * @param reg_msb_set 为 true 时 reg |= 0x80（LIS3DH 多字节读）；MAX30102 单字节读传 false。
 */
ret_code_t i2c_read_regs(uint8_t addr7, uint8_t reg, bool reg_msb_set, uint8_t *data, size_t len);

#endif
