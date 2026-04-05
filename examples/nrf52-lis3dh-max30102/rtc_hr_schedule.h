/**
 * @file rtc_hr_schedule.h
 * @brief LFCLK + RTC：COMPARE0 心率节拍 + TICK 计步采样节拍（同一 RTC2）
 *
 * 使用 RTC 实例 2，避免与部分工程里 RTC0/SoftDevice 占用冲突。
 * 需在 sdk_config.h 启用：RTC_ENABLED、RTC2_ENABLED、NRF_DRV_CLOCK_ENABLED 等。
 */
#ifndef RTC_HR_SCHEDULE_H
#define RTC_HR_SCHEDULE_H

#include <stdint.h>
#include "sdk_errors.h"

/** prescaler=819 → 32768/820 ≈ 40 Hz；用于心率间隔换算与文档说明 */
#define RTC_TICKS_PER_SEC     40u
/** 约 25 ms/次，与原先 main 中 delay 一致（示意整数毫秒） */
#define RTC_MS_PER_TICK       25u

/**
 * 启动 LFCLK、初始化 RTC2、使能 COMPARE0，并首次在 interval_sec 秒后产生节拍。
 */
ret_code_t rtc_hr_schedule_init(uint32_t interval_sec);

/**
 * 从当前 RTC COUNTER 起，再过 interval_sec 秒触发一次 COMPARE0（主循环在测完心率后调用）。
 */
void rtc_hr_schedule_arm_seconds(uint32_t interval_sec);

/**
 * 若 COMPARE0 已发生，清除挂起并返回 true（仅主循环调用）。
 */
bool rtc_hr_schedule_consume_due(void);

/** 丢弃挂起节拍（例如在长时间心率测量期间 RTC 已触发，避免连测两次） */
void rtc_hr_schedule_discard_pending(void);

/**
 * 取出并清零 TICK 累计次数（ISR 里每来一次 TICK +1）。
 * 主循环内对返回值循环读加速度计即可；心率测量阻塞较久时可能一次返回较大 n，属正常。
 */
uint32_t rtc_schedule_drain_step_ticks(void);

#endif
