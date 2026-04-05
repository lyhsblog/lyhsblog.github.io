/**
 * @file rtc_hr_schedule.c
 *
 * prescaler=819 → COUNTER 频率 32768/(819+1) ≈ 40 Hz；
 * COMPARE0 用 RTC_TICKS_PER_SEC(40) 换算「秒」与 COUNTER tick 一致。
 */
#include "rtc_hr_schedule.h"
#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_rtc.h"

/* 32768 / (819+1) ≈ 39.96 Hz → 约 25 ms/TICK，与计步原节拍一致 */
#define RTC_PRESCALER 819u

static const nrf_drv_rtc_t m_rtc = NRF_DRV_RTC_INSTANCE(2);
static volatile bool m_hr_due;
static volatile uint32_t m_step_tick_count;

static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
    if (int_type == NRF_DRV_RTC_INT_TICK) {
        m_step_tick_count++;
    } else if (int_type == NRF_DRV_RTC_INT_COMPARE0) {
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

    nrf_drv_rtc_tick_enable(&m_rtc);
    nrf_drv_rtc_enable(&m_rtc);

    m_hr_due = false;
    m_step_tick_count = 0;
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

uint32_t rtc_schedule_drain_step_ticks(void)
{
    uint32_t n;

    __disable_irq();
    n = m_step_tick_count;
    m_step_tick_count = 0;
    __enable_irq();
    return n;
}
