/**
 * @file rtc_hr_schedule.h
 * @brief LFCLK + RTC2：COMPARE0 = 每分钟任务；TICK ≈25 ms = 计步采样节拍
 */
#ifndef RTC_HR_SCHEDULE_H
#define RTC_HR_SCHEDULE_H

#include <stdint.h>
#include "sdk_errors.h"

#define RTC_TICKS_PER_SEC     40u
#define RTC_MS_PER_TICK       25u

ret_code_t rtc_hr_schedule_init(uint32_t interval_sec);
void rtc_hr_schedule_arm_seconds(uint32_t interval_sec);
bool rtc_hr_schedule_consume_due(void);
void rtc_hr_schedule_discard_pending(void);

/** 主循环取走并清零 TICK 累计（与 ISR 临界） */
uint32_t rtc_schedule_drain_step_ticks(void);

#endif
