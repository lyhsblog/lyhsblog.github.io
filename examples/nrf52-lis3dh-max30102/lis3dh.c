/**
 * @file lis3dh.c
 * @brief ST LIS3DH 最小驱动（I2C）
 */
#include "lis3dh.h"
#include "i2c_bus.h"

#define REG_WHO_AM_I       0x0Fu
#define REG_CTRL_REG1      0x20u
#define REG_CTRL_REG2      0x21u
#define REG_CTRL_REG3      0x22u
#define REG_CTRL_REG4      0x23u
#define REG_REFERENCE      0x26u
#define REG_STATUS_REG     0x27u
#define REG_OUT_X_L        0x28u
#define REG_INT1_CFG       0x30u
#define REG_INT1_SRC       0x31u
#define REG_INT1_THS       0x32u
#define REG_INT1_DURATION  0x33u

static ret_code_t write_reg(uint8_t reg, uint8_t val)
{
    return i2c_write_regs(LIS3DH_I2C_ADDR_7B, reg, &val, 1u);
}

static ret_code_t read_reg(uint8_t reg, uint8_t *v)
{
    /* 单字节读：子地址 bit7=0；多字节读用 read_regs */
    return i2c_read_regs(LIS3DH_I2C_ADDR_7B, reg, false, v, 1u);
}

static ret_code_t read_regs(uint8_t start_reg, uint8_t *dst, size_t len)
{
    return i2c_read_regs(LIS3DH_I2C_ADDR_7B, start_reg, true, dst, len);
}

ret_code_t lis3dh_read_who_am_i(uint8_t *who)
{
    return read_reg(REG_WHO_AM_I, who);
}

ret_code_t lis3dh_init(void)
{
    /* 25 Hz LP，XYZ 使能，LPen=1：与 ST 手册 DR=0101 一致 */
    ret_code_t err = write_reg(REG_CTRL_REG1, 0x5Fu);
    if (err != NRF_SUCCESS) {
        return err;
    }
    /* BDU=1，±2g */
    err = write_reg(REG_CTRL_REG4, 0x08u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_CTRL_REG2, 0x00u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_CTRL_REG3, 0x00u);
    return err;
}

ret_code_t lis3dh_read_accel(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t raw[6];
    ret_code_t err = read_regs(REG_OUT_X_L, raw, sizeof raw);
    if (err != NRF_SUCCESS) {
        return err;
    }
    *x = (int16_t)((int16_t)(raw[1] << 8) | raw[0]);
    *y = (int16_t)((int16_t)(raw[3] << 8) | raw[2]);
    *z = (int16_t)((int16_t)(raw[5] << 8) | raw[4]);
    return NRF_SUCCESS;
}

ret_code_t lis3dh_init_motion_interrupt(void)
{
    ret_code_t err = write_reg(REG_CTRL_REG1, 0x5Fu);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_CTRL_REG4, 0x08u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_REFERENCE, 0x00u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_CTRL_REG2, 0x09u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_INT1_THS, 0x10u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_INT1_DURATION, 0x03u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_INT1_CFG, 0x2Au);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_CTRL_REG3, 0x40u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    (void)lis3dh_clear_int1();
    return NRF_SUCCESS;
}

void lis3dh_clear_int1(void)
{
    uint8_t s = 0;
    (void)read_reg(REG_INT1_SRC, &s);
}
