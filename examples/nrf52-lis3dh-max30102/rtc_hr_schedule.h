/**
 * @file rtc_hr_schedule.h
 * @brief LFCLK + RTC2：COMPARE0 = 每分钟；TICK 可选开关（静止时关 TICK 省电）
 */
#ifndef RTC_HR_SCHEDULE_H
#define RTC_HR_SCHEDULE_H

#include <stdint.h>
#include <stdbool.h>
#include "sdk_errors.h"

#define RTC_TICKS_PER_SEC     40u
#define RTC_MS_PER_TICK       25u

ret_code_t rtc_hr_schedule_init(uint32_t interval_sec);
void rtc_hr_schedule_arm_seconds(uint32_t interval_sec);
bool rtc_hr_schedule_consume_due(void);
void rtc_hr_schedule_discard_pending(void);

uint32_t rtc_schedule_drain_step_ticks(void);

/** 计步采样节拍：true=开 TICK（约 25ms 唤醒）；false=关 TICK，仅 COMPARE0+GPIOTE 可唤醒 */
void rtc_schedule_step_ticks_set(bool enable);

/** 丢弃已挂起的 TICK 计数（关 TICK 前或心率测量前调用） */
void rtc_schedule_step_ticks_flush(void);

/** 当前 RTC COUNTER（24 位），用于运动爆发窗口超时判断 */
uint32_t rtc_schedule_counter_get(void);

#endif
