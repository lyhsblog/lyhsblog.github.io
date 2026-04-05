/**
 * @file rtc_hr_schedule.h
 * @brief LFCLK + RTC2：仅 COMPARE0 按「整分钟」唤醒（无 TICK 计步）
 *
 * prescaler=819 → COUNTER ≈40 Hz，60 s = 2400 tick；用于「每分钟任务」节拍。
 */
#ifndef RTC_HR_SCHEDULE_H
#define RTC_HR_SCHEDULE_H

#include <stdint.h>
#include "sdk_errors.h"

#define RTC_TICKS_PER_SEC     40u

/** 启动 LFCLK、RTC2；首次在 interval_sec 秒后产生第一次 COMPARE0（建议 60） */
ret_code_t rtc_hr_schedule_init(uint32_t interval_sec);

/** 从当前 COUNTER 起再过 interval_sec 秒触发 COMPARE0（每分钟任务结束后调用 60） */
void rtc_hr_schedule_arm_seconds(uint32_t interval_sec);

/** 若 COMPARE0 已到，清标志并返回 true */
bool rtc_hr_schedule_consume_due(void);

/** 心率测量等长临界区内丢弃误触发的 COMPARE，避免结束后立刻再进分钟任务 */
void rtc_hr_schedule_discard_pending(void);

#endif
