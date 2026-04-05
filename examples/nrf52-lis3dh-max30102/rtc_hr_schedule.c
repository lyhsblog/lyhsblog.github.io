/**
 * @file rtc_hr_schedule.c
 *
 * 仅 COMPARE0：用于「每分钟」唤醒；计步由 LIS3DH 运动中断在 RAM 中累加，与本模块无关。
 */
#include "rtc_hr_schedule.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_rtc.h"

#define RTC_PRESCALER 819u

static const nrf_drv_rtc_t m_rtc = NRF_DRV_RTC_INSTANCE(2);
static volatile bool m_minute_due;

static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
    if (int_type == NRF_DRV_RTC_INT_COMPARE0) {
        m_minute_due = true;
    }
}

static void lfclk_blocking_start(void)
{
    ret_code_t err = nrf_drv_clock_init();
    (void)err;
    nrf_drv_clock_lfclk_request(NULL);
    while (!nrf_drv_clock_lfclk_is_running()) {
        /* wait */
    }
}

void rtc_hr_schedule_arm_seconds(uint32_t interval_sec)
{
    uint32_t ticks = interval_sec * RTC_TICKS_PER_SEC;
    if (ticks == 0u) {
        ticks = RTC_TICKS_PER_SEC;
    }

    uint32_t now = nrf_drv_rtc_counter_get(&m_rtc);
    uint32_t next = (now + ticks) & 0x00FFFFFFu;
    nrf_drv_rtc_cc_set(&m_rtc, 0, next, true);
}

ret_code_t rtc_hr_schedule_init(uint32_t interval_sec)
{
    lfclk_blocking_start();

    nrf_drv_rtc_config_t cfg = NRF_DRV_RTC_DEFAULT_CONFIG;
    cfg.prescaler = RTC_PRESCALER;

    ret_code_t err = nrf_drv_rtc_init(&m_rtc, &cfg, rtc_handler);
    if (err != NRF_SUCCESS) {
        return err;
    }

    nrf_drv_rtc_enable(&m_rtc);

    m_minute_due = false;
    rtc_hr_schedule_arm_seconds(interval_sec);
    return NRF_SUCCESS;
}

bool rtc_hr_schedule_consume_due(void)
{
    if (!m_minute_due) {
        return false;
    }
    m_minute_due = false;
    return true;
}

void rtc_hr_schedule_discard_pending(void)
{
    m_minute_due = false;
}
