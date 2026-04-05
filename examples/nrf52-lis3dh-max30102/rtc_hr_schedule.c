/**
 * @file rtc_hr_schedule.c
 */
#include "rtc_hr_schedule.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_rtc.h"

/* 32768 / (4095+1) = 8 Hz */
#define RTC_PRESCALER 4095u

static const nrf_drv_rtc_t m_rtc = NRF_DRV_RTC_INSTANCE(2);
static volatile bool m_hr_due;

static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
    if (int_type == NRF_DRV_RTC_INT_COMPARE0) {
        m_hr_due = true;
    }
}

static void lfclk_blocking_start(void)
{
    ret_code_t err = nrf_drv_clock_init();
    (void)err;

    nrf_drv_clock_lfclk_request(NULL);
    while (!nrf_drv_clock_lfclk_is_running()) {
        /* LFCLK 就绪；无 SoftDevice 时也可 __WFE() */
    }
}

void rtc_hr_schedule_arm_seconds(uint32_t interval_sec)
{
    uint32_t ticks = interval_sec * RTC_HR_TICKS_PER_SEC;
    if (ticks == 0u) {
        ticks = RTC_HR_TICKS_PER_SEC;
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

    m_hr_due = false;
    rtc_hr_schedule_arm_seconds(interval_sec);
    return NRF_SUCCESS;
}

bool rtc_hr_schedule_consume_due(void)
{
    if (!m_hr_due) {
        return false;
    }
    m_hr_due = false;
    return true;
}

void rtc_hr_schedule_discard_pending(void)
{
    m_hr_due = false;
}
