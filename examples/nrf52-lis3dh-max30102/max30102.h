#ifndef MAX30102_H
#define MAX30102_H

#include <stdint.h>
#include <stdbool.h>
#include "sdk_errors.h"

#define MAX30102_I2C_ADDR_7B  0x57u

ret_code_t max30102_init(void);
ret_code_t max30102_read_part_id(uint8_t *part);

void max30102_shutdown(void);
/** HR 模式（红灯），非 Shutdown */
ret_code_t max30102_mode_hr_run(void);

/** 读出 FIFO 中当前样本数并逐条读取（HR 模式每样本 3 字节） */
ret_code_t max30102_drain_fifo(void (*on_red_sample)(uint32_t red18, void *ctx), void *ctx);

/** 将当前 FIFO 内样本读入 red_out，返回实际个数到 *out_count */
ret_code_t max30102_collect_fifo_red(uint32_t *red_out, size_t max_samples, size_t *out_count);

#endif
