/**
 * @file rtc_hr_schedule.h
 * @brief LFCLK + RTC 比较通道：周期性「心率测量」节拍（与主循环分离）
 *
 * 使用 RTC 实例 2，避免与部分工程里 RTC0/SoftDevice 占用冲突。
 * 需在 sdk_config.h 启用：RTC_ENABLED、RTC2_ENABLED、NRF_DRV_CLOCK_ENABLED 等。
 */
#ifndef RTC_HR_SCHEDULE_H
#define RTC_HR_SCHEDULE_H

#include <stdint.h>
#include "sdk_errors.h"

/** LFCLK 32768 Hz、prescaler=4095 → 8 tick/s；间隔秒数 * 8 = 比较增量 */
#define RTC_HR_TICKS_PER_SEC  8u

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

#endif
