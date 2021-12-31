/**
 ******************************************************************************
 * @file    STM32RTC.h
 * @author  Zlobin Alexey
 * @brief   Provides a RTC interface for Arduino
 *
 ******************************************************************************
 */

#ifndef STM32_RTC_H_
#define STM32_RTC_H_

#include "Arduino.h"
#if defined(STM32_CORE_VERSION) && (STM32_CORE_VERSION  > 0x01090000)
	#include "rtc.h"
#endif
// Check if RTC HAL enable in variants/board_name/stm32yzxx_hal_conf.h
#ifndef HAL_RTC_MODULE_ENABLED
	#error "RTC configuration is missing. Check flag HAL_RTC_MODULE_ENABLED in variants/board_name/stm32yzxx_hal_conf.h"
#endif

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef long unsigned int ul32;
typedef unsigned long  u64;
typedef signed char    i8;
typedef signed short   i16;
typedef signed int     i32;
typedef signed long    i64;


typedef struct
{
	u8 hours;
	u8 minutes;
	u8 seconds;
	ul32 sub_seconds;
	u8 year;
	u8 month;
	u8 day;
	u8 wday;
	hourFormat_t format;
	hourAM_PM_t period;

	u8 alarm_day;
	u8 alarm_hours;
	u8 alarm_minutes;
	u8 alarm_seconds;
	ul32 alarm_sub_seconds;
	hourAM_PM_t alarm_period;
	alarmMask_t alarm_mask;
	bool alarm_enabled;

	sourceClock_t source_clock;

	bool _configured;	
	bool _reset;

} STM32RTC;



/**
  * @brief Initialization default values of RTC
  * 
  * @param rtc: RTC
  * @retval None
  */
void
stm32rtc_init(STM32RTC *rtc);


/**
 * @brief Initialize RTC on chip
 * 
 * @param rtc: RTC
 * @param hour_fmt: Hour format: HOUR_FORMAT_24 or HOUR_FORMAT_12
 * @retval None
 */
void
stm32rtc_begin
(
	STM32RTC *rtc,
	hourFormat_t hour_fmt
);


/**
 * @brief Resets and reconfigure RTC
 * 
 * @param rtc: RTC
 * @param hour_fmt: Hour format: HOUR_FORMAT_24 or HOUR_FORMAT_12
 * @retval None
 */
void
stm32rtc_reset
(
	STM32RTC *rtc,
	hourFormat_t hour_fmt
);


/**
 * @brief Deinitialize and stop the RTC
 * 
 * @param rtc: RTC
 * @retval None
 */
void
stm32rtc_end(STM32RTC *rtc);




// GET FUNCTIONS

ul32
stm32rtc_get_sub_seconds(STM32RTC *rtc);

u8
stm32rtc_get_seconds(STM32RTC *rtc);

u8
stm32rtc_get_minutes(STM32RTC *rtc);

u8
stm32rtc_get_hours
(
	STM32RTC *rtc,
	hourAM_PM_t *period
);

void
stm32rtc_get_time
(
	STM32RTC *rtc,
	u8 *hours,
	u8 *minutes,
	u8 *seconds,
	ul32 *sub_seconds,
	hourAM_PM_t *period
);

u8
stm32rtc_get_week_day(STM32RTC *rtc);

u8
stm32rtc_get_day(STM32RTC *rtc);

u8
stm32rtc_get_month(STM32RTC *rtc);

u8
stm32rtc_get_year(STM32RTC *rtc);

void
stm32rtc_get_date
(
	STM32RTC *rtc,
	u8 *wday,
	u8 *day,
	u8 *month,
	u8 *year
);

// SET FUNCTIONS

void
stm32rtc_set_sub_seconds
(
	STM32RTC *rtc,
	ul32 sub_seconds
);

void
stm32rtc_set_seconds
(
	STM32RTC *rtc,
	u8 seconds
);

void
stm32rtc_set_minutes
(
	STM32RTC *rtc,
	u8 minutes
);

void
stm32rtc_set_hours
(
	STM32RTC *rtc,
	u8 hours
);

void
stm32rtc_set_hours_12
(
	STM32RTC *rtc,
	u8 hours,
	hourAM_PM_t period
);

void
stm32rtc_set_time
(
	STM32RTC *rtc,
	u8 hours,
	u8 minutes,
	u8 seconds,
	ul32 sub_seconds
);

void
stm32rtc_set_time_12
(
	STM32RTC *rtc,
	u8 hours,
	u8 minutes,
	u8 seconds,
	ul32 sub_seconds,
	hourAM_PM_t period
);

void
stm32rtc_set_week_day
(
	STM32RTC *rtc,
	u8 wday
);

void
stm32rtc_set_day
(
	STM32RTC *rtc,
	u8 day
);

void
stm32rtc_set_month
(
	STM32RTC *rtc,
	u8 month
);

void
stm32rtc_set_year
(
	STM32RTC *rtc,
	u8 year
);

void
stm32rtc_set_date
(
	STM32RTC *rtc,
	u8 day,
	u8 month,
	u8 year
);

void
stm32rtc_set_wdate
(
	STM32RTC *rtc,
	u8 day,
	u8 month,
	u8 year,
	u8 wday
);


void
stm32rtc_sync_time(STM32RTC *rtc);

void
stm32rtc_sync_date(STM32RTC *rtc);

void
stm32rtc_sync_alarm_time(STM32RTC *rtc);

#endif /* STM32_RTC_H_ */