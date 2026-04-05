/**
 * @file max30102.c
 * @brief Maxim MAX30102 最小驱动（I2C）
 */
#include "max30102.h"
#include "i2c_bus.h"
#include "nrf_delay.h"

#define REG_INT_STATUS1   0x00u
#define REG_INT_STATUS2   0x01u
#define REG_FIFO_WR_PTR   0x04u
#define REG_OVF_COUNTER   0x05u
#define REG_FIFO_RD_PTR   0x06u
#define REG_FIFO_DATA     0x07u
#define REG_FIFO_CONFIG   0x08u
#define REG_MODE_CONFIG   0x09u
#define REG_SPO2_CONFIG   0x0Au
#define REG_LED1_PA       0x0Cu
#define REG_LED2_PA       0x0Du
#define REG_PART_ID       0xFFu

#define MODE_RESET        0x40u
#define MODE_SHDN         0x80u
#define MODE_HR_ONLY      0x02u

static ret_code_t write_reg(uint8_t reg, uint8_t val)
{
    return i2c_write_regs(MAX30102_I2C_ADDR_7B, reg, &val, 1u);
}

static ret_code_t read_reg(uint8_t reg, uint8_t *v)
{
    return i2c_read_regs(MAX30102_I2C_ADDR_7B, reg, false, v, 1u);
}

ret_code_t max30102_read_part_id(uint8_t *part)
{
    return read_reg(REG_PART_ID, part);
}

ret_code_t max30102_init(void)
{
    ret_code_t err = write_reg(REG_MODE_CONFIG, MODE_RESET);
    if (err != NRF_SUCCESS) {
        return err;
    }
    nrf_delay_ms(10);

    err = write_reg(REG_MODE_CONFIG, 0x00u);
    if (err != NRF_SUCCESS) {
        return err;
    }

    uint8_t s1 = 0, s2 = 0;
    (void)read_reg(REG_INT_STATUS1, &s1);
    (void)read_reg(REG_INT_STATUS2, &s2);

    err = write_reg(REG_FIFO_WR_PTR, 0x00u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_OVF_COUNTER, 0x00u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_FIFO_RD_PTR, 0x00u);
    if (err != NRF_SUCCESS) {
        return err;
    }

    err = write_reg(REG_FIFO_CONFIG, 0x4Fu);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_SPO2_CONFIG, 0x27u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_LED1_PA, 0x24u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_LED2_PA, 0x00u);
    if (err != NRF_SUCCESS) {
        return err;
    }

    err = write_reg(REG_MODE_CONFIG, MODE_SHDN);
    return err;
}

void max30102_shutdown(void)
{
    (void)write_reg(REG_MODE_CONFIG, MODE_SHDN);
}

ret_code_t max30102_mode_hr_run(void)
{
    uint8_t s1 = 0, s2 = 0;
    (void)read_reg(REG_INT_STATUS1, &s1);
    (void)read_reg(REG_INT_STATUS2, &s2);

    ret_code_t err = write_reg(REG_FIFO_WR_PTR, 0x00u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_OVF_COUNTER, 0x00u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_FIFO_RD_PTR, 0x00u);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = write_reg(REG_MODE_CONFIG, MODE_HR_ONLY);
    return err;
}

static uint8_t fifo_unread_count(void)
{
    uint8_t w = 0, r = 0;
    (void)read_reg(REG_FIFO_WR_PTR, &w);
    (void)read_reg(REG_FIFO_RD_PTR, &r);
    w &= 0x1Fu;
    r &= 0x1Fu;
    return (uint8_t)((w - r + 32u) & 0x1Fu);
}

ret_code_t max30102_drain_fifo(void (*on_red_sample)(uint32_t red18, void *ctx), void *ctx)
{
    uint8_t n = fifo_unread_count();
    while (n > 0u) {
        uint8_t raw[3];
        ret_code_t err = i2c_read_regs(MAX30102_I2C_ADDR_7B, REG_FIFO_DATA, false, raw, 3u);
        if (err != NRF_SUCCESS) {
            return err;
        }
        uint32_t red = ((uint32_t)raw[0] << 16) | ((uint32_t)raw[1] << 8) | (uint32_t)raw[2];
        red &= 0x0003FFFFu;
        if (on_red_sample != NULL) {
            on_red_sample(red, ctx);
        }
        n = fifo_unread_count();
    }
    return NRF_SUCCESS;
}

typedef struct {
    uint32_t *buf;
    size_t max;
    size_t cnt;
} collect_ctx_t;

static void collect_cb(uint32_t red18, void *ctx)
{
    collect_ctx_t *c = (collect_ctx_t *)ctx;
    if (c->cnt < c->max) {
        c->buf[c->cnt++] = red18;
    }
}

ret_code_t max30102_collect_fifo_red(uint32_t *red_out, size_t max_samples, size_t *out_count)
{
    if (red_out == NULL || out_count == NULL) {
        return NRF_ERROR_NULL;
    }
    collect_ctx_t c = { red_out, max_samples, 0 };
    ret_code_t err = max30102_drain_fifo(collect_cb, &c);
    *out_count = c.cnt;
    return err;
}
