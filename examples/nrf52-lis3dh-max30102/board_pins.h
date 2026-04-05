/**
 * @file board_pins.h
 * @brief 按你的原理图修改 SCL/SDA、LIS3DH INT1。
 * 默认按常见 nRF52840 DK 与飞线调试写法，勿直接用于量产板。
 */
#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "nrf_gpio.h"

/* I2C（LIS3DH + MAX30102 共总线） */
#define BOARD_I2C_SCL_PIN   NRF_GPIO_PIN_MAP(0, 26)
#define BOARD_I2C_SDA_PIN   NRF_GPIO_PIN_MAP(0, 27)

/* LIS3DH INT1 → GPIO + GPIOTE */
#define BOARD_LIS3DH_INT1   NRF_GPIO_PIN_MAP(0, 16)

/* 可选：调试串口（若工程已配 UART，可自行对接 printf） */
/* #define BOARD_UART_TX     NRF_GPIO_PIN_MAP(0, 6) */

#endif /* BOARD_PINS_H */
