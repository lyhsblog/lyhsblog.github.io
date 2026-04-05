/**
 * @file i2c_bus.c
 * @brief nRF5 SDK nrf_drv_twi（TWI 主机）
 *
 * 需在 sdk_config.h 中启用：TWI0_USE_EASY_DMA、TWI_ENABLED、TWI0_ENABLED 等。
 */
#include "i2c_bus.h"
#include "board_pins.h"
#include "nrf_drv_twi.h"

static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(0);
static bool m_inited;

ret_code_t i2c_bus_init(void)
{
    if (m_inited) {
        return NRF_SUCCESS;
    }

    nrf_drv_twi_config_t cfg = NRF_DRV_TWI_DEFAULT_CONFIG;
    cfg.scl       = BOARD_I2C_SCL_PIN;
    cfg.sda       = BOARD_I2C_SDA_PIN;
    cfg.frequency = NRF_DRV_TWI_FREQ_400K;

    ret_code_t err = nrf_drv_twi_init(&m_twi, &cfg, NULL, NULL);
    if (err != NRF_SUCCESS) {
        return err;
    }
    nrf_drv_twi_enable(&m_twi);
    m_inited = true;
    return NRF_SUCCESS;
}

void i2c_bus_uninit(void)
{
    if (!m_inited) {
        return;
    }
    nrf_drv_twi_disable(&m_twi);
    nrf_drv_twi_uninit(&m_twi);
    m_inited = false;
}

ret_code_t i2c_write_regs(uint8_t addr7, uint8_t reg, uint8_t const *data, size_t len)
{
    if (len > 16u) {
        return NRF_ERROR_INVALID_LENGTH;
    }
    uint8_t buf[17];
    buf[0] = reg;
    for (size_t i = 0; i < len; i++) {
        buf[1 + i] = data[i];
    }
    ret_code_t err = nrf_drv_twi_tx(&m_twi, addr7, buf, (uint8_t)(len + 1u), false);
    return err;
}

ret_code_t i2c_read_regs(uint8_t addr7, uint8_t reg, bool reg_msb_set, uint8_t *data, size_t len)
{
    uint8_t r = reg_msb_set ? (uint8_t)(reg | 0x80u) : reg;

    ret_code_t err = nrf_drv_twi_tx(&m_twi, addr7, &r, 1, true);
    if (err != NRF_SUCCESS) {
        return err;
    }
    err = nrf_drv_twi_rx(&m_twi, addr7, data, len);
    return err;
}
