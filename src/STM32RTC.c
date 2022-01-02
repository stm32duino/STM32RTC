/**
 ******************************************************************************
 * @file    STM32RTC.c
 * @author  Zlobin Alexey
 * @brief   Provides a RTC interface for Arduino
 * *
 ******************************************************************************
 */


#include "STM32RTC.h"

// CONTROL FUNCTION

#define EPOCH_TIME_OFF      946684800  // This is 1st January 2000, 00:00:00 in epoch time
#define EPOCH_TIME_YEAR_OFF 100        // years since 1900

void
stm32rtc_init
(
	STM32RTC *rtc
)
{
	rtc->_configured = false;
	rtc->_reset = false;
	rtc->source_clock = LSI_CLOCK;
}

void
stm32rtc_begin
(
	STM32RTC *rtc,
	hourFormat_t h_fmt
)
{
	if (!rtc->_configured)
	{
		rtc->format = h_fmt;
		RTC_init
		(
			h_fmt,
			rtc->source_clock,
			rtc->_reset
		);
		rtc->_configured = true;
		stm32rtc_sync_time(rtc);
		stm32rtc_sync_date(rtc);
		rtc->alarm_day = rtc->day;
		rtc->alarm_hours = rtc->hours;
		rtc->alarm_minutes = rtc->minutes;
		rtc->alarm_seconds = rtc->seconds;
		rtc->alarm_sub_seconds = rtc->sub_seconds;
		rtc->alarm_period = rtc->period;
	}
	else
	{
		stm32rtc_sync_time(rtc);
		stm32rtc_sync_date(rtc);
		stm32rtc_sync_alarm_time(rtc);
	}
}

void
stm32rtc_reset
(
	STM32RTC *rtc,
	hourFormat_t hour_fmt
)
{
	rtc->_reset = true;
	rtc->_configured = false;
	rtc->alarm_enabled = false;
	stm32rtc_begin(rtc, hour_fmt);
}

void
stm32rtc_end(STM32RTC *rtc)
{
	if (rtc->_configured)
	{
		RTC_DeInit();
		rtc->_configured = false;
		rtc->alarm_enabled = false;
	}
}

void
stm32rtc_enable_alarm
(
	STM32RTC *rtc,
	alarmMask_t mask
)
{
	if (rtc->_configured)
	{
		rtc->alarm_mask = mask;
		switch (mask)
		{
			case OFF_MSK:
				RTC_StopAlarm();
				break;
			case SS_MSK:
			case MM_MSK:
			case HH_MSK:
			case D_MSK:
			case M_MSK:
			case Y_MSK:
				RTC_StartAlarm
				(
					rtc->alarm_day,
					rtc->alarm_hours,
					rtc->alarm_minutes,
					rtc->alarm_seconds,
					rtc->alarm_sub_seconds,
					rtc->alarm_period,
					rtc->alarm_mask
				);
				rtc->alarm_enabled = true;
				break;
			default:
				break;
		}
	}
}

void
stm32rtc_disable_alarm(STM32RTC *rtc)
{
	if (rtc->_configured)
	{
		RTC_StopAlarm();
		rtc->alarm_enabled = false;
	}
}

void
stm32rtc_attach_intrerrupt
(
	STM32RTC *rtc,
	voidCallbackPtr callback,
	void *data
)
{
	attachAlarmCallback(callback, data);
}

void
stm32rtc_detach_intrerrupt(STM32RTC *rtc)
{
	detachAlarmCallback();
}

#ifdef ONESECOND_IRQn

	void
	stm32rtc_attach_secconds_intrerrupt
	(
		STM32RTC *rtc,
		voidCallbackPtr callback
	)
	{
		attachSecondsIrqCallback(callback);
	}

	void
	stm32rtc_detach_seconds_intrerrupt(STM32RTC *rtc)
	{
		detachSecondsIrqCallback();
	}

#endif /* ONESCOND_IRQn */



// GET FUNCTIONS

sourceClock_t
stm32rtc_get_source_clock(STM32RTC *rtc)
{
	return rtc->source_clock;
}

ul32
stm32rtc_get_sub_seconds(STM32RTC *rtc)
{
	stm32rtc_sync_time(rtc);
	return rtc->sub_seconds;
}

u8
stm32rtc_get_seconds(STM32RTC *rtc)
{
	stm32rtc_sync_time(rtc);
	return rtc->seconds;
}

u8
stm32rtc_get_minutes(STM32RTC *rtc)
{
	stm32rtc_sync_time(rtc);
	return rtc->minutes;
}

u8
stm32rtc_get_hours
(
	STM32RTC *rtc,
	hourAM_PM_t *period
)
{
	stm32rtc_sync_time(rtc);
	if (period != NULL)
		*period = rtc->period;
	return rtc->hours;
}

void
stm32rtc_get_time
(
	STM32RTC *rtc,
	u8 *hours,
	u8 *minutes,
	u8 *seconds,
	ul32 *sub_seconds,
	hourAM_PM_t *period
)
{
	stm32rtc_sync_time(rtc);
	if (hours != NULL)
		*hours = rtc->hours;
	if (minutes != NULL)
		*minutes = rtc->minutes;
	if (seconds != NULL)
		*seconds = rtc->seconds;
	if (sub_seconds != NULL)
		*sub_seconds = rtc->sub_seconds;
	if (period != NULL)
		*period = rtc->period;
}

u8
stm32rtc_get_week_day(STM32RTC *rtc)
{
	stm32rtc_sync_date(rtc);
	return rtc->wday;
}

u8
stm32rtc_get_day(STM32RTC *rtc)
{
	stm32rtc_sync_date(rtc);
	return rtc->day;
}

u8
stm32rtc_get_month(STM32RTC *rtc)
{
	stm32rtc_sync_date(rtc);
	return rtc->month;
}

u8
stm32rtc_get_year(STM32RTC *rtc)
{
	stm32rtc_sync_date(rtc);
	return rtc->year;
}

void
stm32rtc_get_date
(
	STM32RTC *rtc,
	u8 *wday,
	u8 *day,
	u8 *month,
	u8 *year
)
{
	stm32rtc_sync_date(rtc);
	if (wday != NULL)
		*wday = rtc->wday;
	if (day != NULL)
		*day = rtc->day;
	if (month != NULL)
		*month = rtc->month;
	if (year != NULL)
		*year = rtc->year;
}

ul32
stm32rtc_get_alarm_sub_seconds(STM32RTC *rtc)
{
	stm32rtc_sync_alarm_time(rtc);
	return rtc->alarm_sub_seconds;
}

u8
stm32rtc_get_alarm_seconds(STM32RTC *rtc)
{
	stm32rtc_sync_alarm_time(rtc);
	return rtc->alarm_seconds;
}

u8
stm32rtc_get_alarm_minutes(STM32RTC *rtc)
{
	stm32rtc_sync_alarm_time(rtc);
	return rtc->alarm_minutes;
}

u8
stm32rtc_get_alarm_hours
(
	STM32RTC *rtc,
	hourAM_PM_t *period
)
{
	stm32rtc_sync_alarm_time(rtc);
	if (period != NULL)
		*period = rtc->alarm_period;
	return rtc->alarm_hours;
}

u8
stm32rtc_get_alarm_day(STM32RTC *rtc)
{
	stm32rtc_sync_alarm_time(rtc);
	return rtc->alarm_day;
}

void
stm32rtc_get_alarm
(
	STM32RTC *rtc,
	u8 *day,
	u8 *hours,
	u8 *minutes,
	u8 *seconds,
	ul32 *sub_seconds,
	hourAM_PM_t *period
)
{
	stm32rtc_sync_alarm_time(rtc);
	if (hours != NULL)
		*hours = rtc->alarm_hours;
	if (minutes != NULL)
		*minutes = rtc->alarm_minutes;
	if (seconds != NULL)
		*seconds = rtc->alarm_seconds;
	if (sub_seconds != NULL)
		*sub_seconds = rtc->alarm_sub_seconds;
	if (period != NULL)
		*period = rtc->alarm_period;
	if (day != NULL)
		*day = rtc->alarm_day;
}


// SET FUNCTIONS

void
stm32rtc_set_source_clock
(
	STM32RTC *rtc,
	sourceClock_t source
)
{
	if (IS_CLOCK_SOURCE(source))
	{
		rtc->source_clock = source;
		RTC_SetClockSource(source);
	}
}

void
stm32rtc_set_sub_seconds
(
	STM32RTC *rtc,
	ul32 sub_seconds
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_time(rtc);
		if (sub_seconds < 1000)
			rtc->sub_seconds = sub_seconds;
		RTC_SetTime
		(
			rtc->hours,
			rtc->minutes,
			rtc->seconds,
			rtc->sub_seconds,
			rtc->period
		);
	}
}

void
stm32rtc_set_seconds
(
	STM32RTC *rtc,
	u8 seconds
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_time(rtc);
		if (seconds < 60)
			rtc->seconds = seconds;
		RTC_SetTime
		(
			rtc->hours,
			rtc->minutes,
			rtc->seconds,
			rtc->sub_seconds,
			rtc->period
		);
	}
}

void
stm32rtc_set_minutes
(
	STM32RTC *rtc,
	u8 minutes
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_time(rtc);
		if (minutes < 60)
			rtc->minutes = minutes;
		RTC_SetTime
		(
			rtc->hours,
			rtc->minutes,
			rtc->seconds,
			rtc->sub_seconds,
			rtc->period
		);
	}
}

void
stm32rtc_set_hours
(
	STM32RTC *rtc,
	u8 hours
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_time(rtc);
		if (hours < 24)
			rtc->hours = hours;
		RTC_SetTime
		(
			rtc->hours,
			rtc->minutes,
			rtc->seconds,
			rtc->sub_seconds,
			rtc->period
		);
	}
}

void
stm32rtc_set_hours_12
(
	STM32RTC *rtc,
	u8 hours,
	hourAM_PM_t period
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_time(rtc);
		if (hours < 12)
			rtc->hours = hours;
		if (rtc->format == HOUR_FORMAT_12)
			rtc->period = period;
		RTC_SetTime
		(
			rtc->hours,
			rtc->minutes,
			rtc->seconds,
			rtc->sub_seconds,
			rtc->period
		);
	}
}

void
stm32rtc_set_time
(
	STM32RTC *rtc,
	u8 hours,
	u8 minutes,
	u8 seconds,
	ul32 sub_seconds
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_time(rtc);
		if (hours < 24)
			rtc->hours = hours;
		if (minutes < 60)
			rtc->minutes = minutes;
		if (seconds < 60)
			rtc->seconds = seconds;
		if (sub_seconds < 1000)
			rtc->sub_seconds = sub_seconds;
		RTC_SetTime
		(
			rtc->hours,
			rtc->minutes,
			rtc->seconds,
			rtc->sub_seconds,
			rtc->period
		);
	}
}

void
stm32rtc_set_time_12
(
	STM32RTC *rtc,
	u8 hours,
	u8 minutes,
	u8 seconds,
	ul32 sub_seconds,
	hourAM_PM_t period
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_time(rtc);
		if (hours < 24)
			rtc->hours = hours;
		if (minutes < 60)
			rtc->minutes = minutes;
		if (seconds < 60)
			rtc->seconds = seconds;
		if (sub_seconds < 1000)
			rtc->sub_seconds = sub_seconds;
		if (rtc->format == HOUR_FORMAT_12)
			rtc->period = period;
		RTC_SetTime
		(
			rtc->hours,
			rtc->minutes,
			rtc->seconds,
			rtc->sub_seconds,
			rtc->period
		);
	}

}

void
stm32rtc_set_week_day
(
	STM32RTC *rtc,
	u8 wday
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_date(rtc);
		if (wday >= 1 && wday <= 7)
			rtc->wday = wday;
		RTC_SetDate
		(
			rtc->year,
			rtc->month,
			rtc->day,
			rtc->wday
		);
	}
}

void
stm32rtc_set_day
(
	STM32RTC *rtc,
	u8 day
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_date(rtc);
		if (day >= 1 && day <= 31)
			rtc->day = day;
		RTC_SetDate
		(
			rtc->year,
			rtc->month,
			rtc->day,
			rtc->wday
		);
	}
}

void
stm32rtc_set_month
(
	STM32RTC *rtc,
	u8 month
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_date(rtc);
		if (month >= 1 && month <= 12)
			rtc->month = month;
		RTC_SetDate
		(
			rtc->year,
			rtc->month,
			rtc->day,
			rtc->wday
		);
	}
}

void
stm32rtc_set_year
(
	STM32RTC *rtc,
	u8 year
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_date(rtc);
		if (year < 100)
			rtc->year = year;
		RTC_SetDate
		(
			rtc->year,
			rtc->month,
			rtc->day,
			rtc->wday
		);
	}
}

void
stm32rtc_set_date
(
	STM32RTC *rtc,
	u8 day,
	u8 month,
	u8 year
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_date(rtc);
		if (day >= 1 && day <= 31)
			rtc->day = day;
		if (month >= 1 && month <= 12)
			rtc->month = month;
		if (year < 100)
			rtc->year = year;
		RTC_SetDate
		(
			rtc->year,
			rtc->month,
			rtc->day,
			rtc->wday
		);
	}
}

void
stm32rtc_set_wdate
(
	STM32RTC *rtc,
	u8 day,
	u8 month,
	u8 year,
	u8 wday
)
{
	if (rtc->_configured)
	{
		stm32rtc_sync_date(rtc);
		if (wday >= 1 && wday <= 7)
			rtc->wday = wday;
		if (day >= 1 && day <= 31)
			rtc->day = day;
		if (month >= 1 && month <= 12)
			rtc->month = month;
		if (year < 100)
			rtc->year = year;
		RTC_SetDate
		(
			rtc->year,
			rtc->month,
			rtc->day,
			rtc->wday
		);
	}
}

void
stm32rtc_set_alarm_sub_seconds
(
	STM32RTC *rtc,
	ul32 sub_seconds
)
{
	if (rtc->_configured)
		if (sub_seconds < 1000)
			rtc->alarm_sub_seconds = sub_seconds;
}

void
stm32rtc_set_alarm_seconds
(
	STM32RTC *rtc,
	u8 seconds
)
{
	if (rtc->_configured)
		if (seconds < 60)
			rtc->alarm_seconds = seconds;
}

void
stm32rtc_set_alarm_minutes
(
	STM32RTC *rtc,
	u8 minutes
)
{
	if (rtc->_configured)
		if (minutes < 60)
		 rtc->alarm_minutes = minutes;
}

void
stm32rtc_set_alarm_hours
(
	STM32RTC *rtc,
	u8 hours
)
{
	if (rtc->_configured)
		if (hours < 24)
			rtc->alarm_hours = hours;
}

void
stm32rtc_set_alarm_hours_12
(
	STM32RTC *rtc,
	u8 hours,
	hourAM_PM_t period
)
{
	if (rtc->_configured)
	{
		if (hours < 12)
			rtc->alarm_hours = hours;
		if (rtc->format == HOUR_FORMAT_12)
			rtc->alarm_period = period;
	}
}

void
stm32rtc_set_alarm_day
(
	STM32RTC *rtc,
	u8 day
)
{
	if (rtc->_configured)
		if (day >= 1 && day <= 31)
			rtc->alarm_day = day;
}

void
stm32rtc_set_alarm
(
	STM32RTC *rtc,
	u8 day,
	u8 hours,
	u8 minutes,
	u8 seconds,
	ul32 sub_seconds
)
{
	stm32rtc_set_alarm_day(rtc, day);	
	stm32rtc_set_alarm_hours(rtc, hours);
	stm32rtc_set_alarm_minutes(rtc, minutes);
	stm32rtc_set_alarm_seconds(rtc, seconds);
	stm32rtc_set_alarm_sub_seconds(rtc, sub_seconds);
}

void
stm32rtc_set_alarm_12
(
	STM32RTC *rtc,
	u8 day,
	u8 hours,
	u8 minutes,
	u8 seconds,
	ul32 sub_seconds,
	hourAM_PM_t period
)
{
	stm32rtc_set_alarm_day(rtc, day);	
	stm32rtc_set_alarm_hours_12(rtc, hours, period);
	stm32rtc_set_alarm_minutes(rtc, minutes);
	stm32rtc_set_alarm_seconds(rtc, seconds);
	stm32rtc_set_alarm_sub_seconds(rtc, sub_seconds);
}



// EPOCH FUNCTIONS

ul32
stm32rtc_get_epoch
(
	STM32RTC *rtc,
	ul32 *subseconds
)
{
	struct tm tm;

	stm32rtc_sync_time(rtc);
	stm32rtc_sync_date(rtc);

	tm.tm_isdst = -1;
	tm.tm_yday = 0;
	tm.tm_wday = 0;
	tm.tm_year = rtc->year + EPOCH_TIME_YEAR_OFF;
	tm.tm_mon = rtc->month - 1;
	tm.tm_mday = rtc->day;
	tm.tm_hour = rtc->hours;
	tm.tm_min = rtc->minutes;
	tm.tm_sec = rtc->seconds;
	if (subseconds != NULL)
		*subseconds = rtc->sub_seconds;

	return mktime(&tm);
}

ul32
stm32rtc_get_y2k_epoch(STM32RTC *rtc)
{
	return (stm32rtc_get_epoch(rtc, NULL) - EPOCH_TIME_OFF);
}

void
stm32rtc_set_epoch
(
	STM32RTC *rtc,
	ul32 ts,
	ul32 sub_seconds
)
{
	if (rtc->_configured)
	{
		if (ts < EPOCH_TIME_OFF)
			ts = EPOCH_TIME_OFF;

		time_t t = ts;
		struct tm *tmp = gmtime(&t);

		rtc->year = tmp->tm_year - EPOCH_TIME_YEAR_OFF;
		rtc->month = tmp->tm_mon + 1;
		rtc->day = tmp->tm_mday;
		if (tmp->tm_wday == 0) {
			rtc->wday = RTC_WEEKDAY_SUNDAY;
		} else {
			rtc->wday = tmp->tm_wday;
		}
		rtc->hours = tmp->tm_hour;
		rtc->minutes = tmp->tm_min;
		rtc->seconds = tmp->tm_sec;
		rtc->sub_seconds = sub_seconds;

		RTC_SetDate
		(
			rtc->year,
			rtc->month,
			rtc->day,
			rtc->wday
		);

		RTC_SetTime
		(
			rtc->hours,
			rtc->minutes,
			rtc->seconds,
			rtc->sub_seconds,
			rtc->period
		);
	}
}

void
stm32rtc_set_y2k_epoch
(
	STM32RTC *rtc,
	ul32 ts
)
{
	if (rtc->_configured)
		stm32rtc_set_epoch(rtc, ts + EPOCH_TIME_OFF, 0);
}

void
stm32rtc_set_alarm_epoch
(
	STM32RTC *rtc,
	ul32 ts,
	alarmMask_t mask,
	ul32 sub_seconds
)
{
	if (rtc->_configured)
	{
		if (ts < EPOCH_TIME_OFF)
			ts = EPOCH_TIME_OFF;

		time_t t = ts;
		struct tm *tmp = gmtime(&t);

		stm32rtc_set_alarm_day(rtc, tmp->tm_mday);
		stm32rtc_set_alarm_hours(rtc, tmp->tm_hour);
		stm32rtc_set_alarm_minutes(rtc, tmp->tm_min);
		stm32rtc_set_alarm_seconds(rtc, tmp->tm_sec);
		stm32rtc_set_alarm_sub_seconds(rtc, sub_seconds);
		stm32rtc_enable_alarm(rtc, mask);
	}
}

#ifdef STM32F1xx

	void
	stm32rtc_get_prediv
	(
		STM32RTC *rtc,
		ul32 *prediv_a,
		i16 *dummy
	)
	{
		(void)dummy;
		RTC_getPrediv(prediv_a);
	}

	void
	stm32rtc_set_prediv
	(
		STM32RTC *rtc,
		ul32 prediv_a,
		i16 dummy
	)
	{
		(void)dummy;
		RTC_setPrediv(prediv_a);
	}
#else

	void
	stm32rtc_get_prediv
	(
		STM32RTC *rtc,
		i8 *prediv_a,
		i16* prediv_s
	)
	{
		if (prediv_a != NULL && prediv_s != NULL)
			RTC_getPrediv(prediv_a, prediv_s);
	}

	void
	stm32rtc_set_prediv
	(
		STM32RTC *rtc,
		i8 prediv_a,
		i16 prediv_s
	)
	{
		RTC_setPrediv(prediv_a, prediv_s);
	}
#endif /* STM32F1xx */


bool
stm32rtc_is_configured(STM32RTC *rtc)
{
	return rtc->_configured;
}

bool
stm32rtc_is_alarm_enabled(STM32RTC *rtc)
{
	return rtc->alarm_enabled;
}

bool
stm32rtc_is_time_set(STM32RTC *rtc)
{
	#if defined(STM32_CORE_VERSION) && (STM32_CORE_VERSION  > 0x01050000)
		return RTC_IsTimeSet();
	#else
		return false;	
	#endif
}








void
stm32rtc_sync_time(STM32RTC *rtc)
{
	if (rtc->_configured)
	{
		RTC_GetTime
		(
			&(rtc->hours),
			&(rtc->minutes),
			&(rtc->seconds),
			&(rtc->sub_seconds),
			&(rtc->period)
		);
	}
}

void
stm32rtc_sync_date(STM32RTC *rtc)
{
	if (rtc->_configured)
	{
		RTC_GetDate
		(
			&(rtc->year),
			&(rtc->month),
			&(rtc->day),
			&(rtc->wday)
		);
	}
}

void
stm32rtc_sync_alarm_time(STM32RTC *rtc)
{
	if (rtc->_configured)
	{
		u8 mask;
		RTC_GetAlarm
		(
			&(rtc->alarm_day),
			&(rtc->alarm_hours),
			&(rtc->alarm_minutes),
			&(rtc->alarm_seconds),
			&(rtc->alarm_sub_seconds),
			&(rtc->alarm_period),
			&(rtc->alarm_mask)
		);
	}
}
