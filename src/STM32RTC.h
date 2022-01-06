/**
  ******************************************************************************
  * @file    STM32RTC.h
  * @author  Frederic Pillon
  * @brief   Provides a RTC interface for Arduino
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2020 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#if defined(__cplusplus) && !defined(C_IMPLEMENTATION)


#ifndef __STM32_RTC_H
#define __STM32_RTC_H

#include "Arduino.h"
#if defined(STM32_CORE_VERSION) && (STM32_CORE_VERSION  > 0x01090000)
  #include "rtc.h"
#endif
// Check if RTC HAL enable in variants/board_name/stm32yzxx_hal_conf.h
#ifndef HAL_RTC_MODULE_ENABLED
  #error "RTC configuration is missing. Check flag HAL_RTC_MODULE_ENABLED in variants/board_name/stm32yzxx_hal_conf.h"
#endif

/**
 * @brief STM32 RTC library version number
 */
#define STM32_RTC_VERSION_MAJOR    (0x01U) /*!< [31:24] major version */
#define STM32_RTC_VERSION_MINOR    (0x01U) /*!< [23:16] minor version */
#define STM32_RTC_VERSION_PATCH    (0x01U) /*!< [15:8]  patch version */
/*
 * Extra label for development:
 * 0: official release
 * [1-9]: release candidate
 * F[0-9]: development
 */
#define STM32_RTC_VERSION_EXTRA    (0x00U) /*!< [7:0]  extra version */
#define STM32_RTC_VERSION          ((STM32_RTC_VERSION_MAJOR << 24U)\
                                        |(STM32_RTC_VERSION_MINOR << 16U)\
                                        |(STM32_RTC_VERSION_PATCH << 8U )\
                                        |(STM32_RTC_VERSION_EXTRA))

typedef void(*voidFuncPtr)(void *);

#define IS_CLOCK_SOURCE(SRC) (((SRC) == STM32RTC::LSI_CLOCK) || ((SRC) == STM32RTC::LSE_CLOCK) ||\
                              ((SRC) == STM32RTC::HSE_CLOCK))
#define IS_HOUR_FORMAT(FMT)  (((FMT) == STM32RTC::HOUR_12) || ((FMT) == STM32RTC::HOUR_24))

class STM32RTC {
  public:

    enum Hour_Format : uint8_t {
      HOUR_12 = HOUR_FORMAT_12,
      HOUR_24 = HOUR_FORMAT_24
    };

    enum AM_PM : uint8_t {
      AM = HOUR_AM,
      PM = HOUR_PM
    };

    enum Alarm_Match : uint8_t {
      MATCH_OFF          = OFF_MSK,                          // Never
      MATCH_SS           = SS_MSK,                           // Every Minute
      MATCH_MMSS         = SS_MSK | MM_MSK,                  // Every Hour
      MATCH_HHMMSS       = SS_MSK | MM_MSK | HH_MSK,         // Every Day
      MATCH_DHHMMSS      = SS_MSK | MM_MSK | HH_MSK | D_MSK, // Every Month
      /* NOTE: STM32 RTC can't assign a month or a year to an alarm. Those enum
      are kept for compatibility but are ignored inside enableAlarm(). */
      MATCH_MMDDHHMMSS   = SS_MSK | MM_MSK | HH_MSK | D_MSK | M_MSK,
      MATCH_YYMMDDHHMMSS = SS_MSK | MM_MSK | HH_MSK | D_MSK | M_MSK | Y_MSK
    };

    enum Source_Clock : uint8_t {
      LSI_CLOCK = ::LSI_CLOCK,
      LSE_CLOCK = ::LSE_CLOCK,
      HSE_CLOCK = ::HSE_CLOCK
    };

    static STM32RTC &getInstance()
    {
      static STM32RTC instance; // Guaranteed to be destroyed.
      // Instantiated on first use.
      return instance;
    }

    STM32RTC(STM32RTC const &)        = delete;
    void operator=(STM32RTC const &)  = delete;

    void begin(bool resetTime, Hour_Format format = HOUR_24);
    void begin(Hour_Format format = HOUR_24);

    void end(void);

    Source_Clock getClockSource(void);
    void setClockSource(Source_Clock source);

    void enableAlarm(Alarm_Match match);
    void disableAlarm(void);

    void attachInterrupt(voidFuncPtr callback, void *data = nullptr);
    void detachInterrupt(void);

#ifdef ONESECOND_IRQn
    // Other mcu than stm32F1 will use the WakeUp feature to interrupt each second.
    void attachSecondsInterrupt(voidFuncPtr callback);
    void detachSecondsInterrupt(void);

#endif /* ONESECOND_IRQn */
    // Kept for compatibility: use STM32LowPower library.
    void standbyMode();

    /* Get Functions */

    uint32_t getSubSeconds(void);
    uint8_t getSeconds(void);
    uint8_t getMinutes(void);
    uint8_t getHours(AM_PM *period = nullptr);
    void getTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds, uint32_t *subSeconds, AM_PM *period = nullptr);

    uint8_t getWeekDay(void);
    uint8_t getDay(void);
    uint8_t getMonth(void);
    uint8_t getYear(void);
    void getDate(uint8_t *weekDay, uint8_t *day, uint8_t *month, uint8_t *year);

    uint32_t getAlarmSubSeconds(void);
    uint8_t getAlarmSeconds(void);
    uint8_t getAlarmMinutes(void);
    uint8_t getAlarmHours(AM_PM *period = nullptr);

    uint8_t getAlarmDay(void);

    // Kept for compatibility with Arduino RTCZero library.
    uint8_t getAlarmMonth();
    uint8_t getAlarmYear();

    /* Set Functions */

    void setSubSeconds(uint32_t subSeconds);
    void setSeconds(uint8_t seconds);
    void setMinutes(uint8_t minutes);
    void setHours(uint8_t hours, AM_PM period = AM);
    void setTime(uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t subSeconds = 1000, AM_PM period = AM);

    void setWeekDay(uint8_t weekDay);
    void setDay(uint8_t day);
    void setMonth(uint8_t month);
    void setYear(uint8_t year);
    void setDate(uint8_t day, uint8_t month, uint8_t year);
    void setDate(uint8_t weekDay, uint8_t day, uint8_t month, uint8_t year);

    void setAlarmSubSeconds(uint32_t subSeconds);
    void setAlarmSeconds(uint8_t seconds);
    void setAlarmMinutes(uint8_t minutes);
    void setAlarmHours(uint8_t hours, AM_PM period = AM);
    void setAlarmTime(uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t subSeconds = 0, AM_PM period = AM);

    void setAlarmDay(uint8_t day);

    // Kept for compatibility with Arduino RTCZero library.
    void setAlarmMonth(uint8_t month);
    void setAlarmYear(uint8_t year);
    void setAlarmDate(uint8_t day, uint8_t month, uint8_t year);

    /* Epoch Functions */

    uint32_t getEpoch(uint32_t *subSeconds = nullptr);
    uint32_t getY2kEpoch(void);
    void setEpoch(uint32_t ts, uint32_t subSeconds = 0);
    void setY2kEpoch(uint32_t ts);
    void setAlarmEpoch(uint32_t ts, Alarm_Match match = MATCH_DHHMMSS, uint32_t subSeconds = 0);

#if defined(STM32F1xx)
    void getPrediv(uint32_t *predivA, int16_t *dummy = nullptr);
    void setPrediv(uint32_t predivA, int16_t dummy = 0);
#else
    void getPrediv(int8_t *predivA, int16_t *predivS);
    void setPrediv(int8_t predivA, int16_t predivS);
#endif /* STM32F1xx */
    bool isConfigured(void)
    {
      return _configured;
    }
    bool isAlarmEnabled(void)
    {
      return _alarmEnabled;
    }
    bool isTimeSet(void)
    {
#if defined(STM32_CORE_VERSION) && (STM32_CORE_VERSION  > 0x01050000)
      return RTC_IsTimeSet();
#else
      return false;
#endif
    }

    friend class STM32LowPower;

  private:
    STM32RTC(void): _clockSource(LSI_CLOCK) {}

    static bool _configured;
    static bool _reset;

    Hour_Format _format;
    AM_PM       _hoursPeriod;
    uint8_t     _hours;
    uint8_t     _minutes;
    uint8_t     _seconds;
    uint32_t    _subSeconds;
    uint8_t     _year;
    uint8_t     _month;
    uint8_t     _day;
    uint8_t     _wday;

    uint8_t     _alarmDay;
    uint8_t     _alarmHours;
    uint8_t     _alarmMinutes;
    uint8_t     _alarmSeconds;
    uint32_t    _alarmSubSeconds;
    AM_PM       _alarmPeriod;
    Alarm_Match _alarmMatch;
    bool        _alarmEnabled;

    Source_Clock _clockSource;

    void configForLowPower(Source_Clock source);

    void syncTime(void);
    void syncDate(void);
    void syncAlarmTime(void);

};

#endif // __STM32_RTC_H


#else /* C IMPLENTATION */

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
	#include "time.h"
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


#define IS_CLOCK_SOURCE(SRC) (((SRC) == LSI_CLOCK) || ((SRC) == LSE_CLOCK) || \
                              ((SRC) == HSI_CLOCK) || ((SRC) == HSE_CLOCK))
#define IS_HOUR_FORMAT(FMT)  (((FMT) == HOUR_FORMAT_12) || ((FMT) == HOUR_FORMAT_24))

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



// CONTROL FUNCTIONS

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

/**
 * @brief Enable RTC alarm
 * 
 * @param rtc: RTC
 * @param mask: Alarm configuration
 * @retval None
 */
void
stm32rtc_enable_alarm
(
	STM32RTC *rtc,
	alarmMask_t mask
);

/**
 * @brief Disable RTC alarm
 * 
 * @param rtc: RTC
 * @retval None
 */
void
stm32rtc_disable_alarm(STM32RTC *rtc);

/**
 * @brief Attach callback to RTC allarm intrerrupt
 * 
 * @param rtc: RTC
 * @param callback: Pointer to callback
 * @param data: Data
 * @retval None
 */
void
stm32rtc_attach_intrerrupt
(
	STM32RTC *rtc,
	voidCallbackPtr callback,
	void *data
);

/**
 * @brief Detach RTC alarm callback
 * 
 * @param rtc: RTC
 * @retval None
 */
void
stm32rtc_detach_intrerrupt(STM32RTC *rtc);

#ifdef ONESECOND_IRQn
	// Other mcu than stm32F1 will use the WakeUp feature to interrupt each second.

	/**
	 * @brief Attach callback to RTC Seconds intreerrupt
	 * 
	 * @param rtc: RTC
	 * @param callback: Pointer to callback
	 * @retval None
	 */
	void
	stm32rtc_attach_secconds_intrerrupt
	(
		STM32RTC *rtc,
		voidCallbackPtr callback
	);

	/**
	 * @brief Detach callback to RTC Seconds intrerrupt
	 * 
	 * @param rtc: RTC
	 *
	 */
	void
	stm32rtc_detach_seconds_intrerrupt(STM32RTC *rtc);

#endif /* ONESECOND_IRQn */







// GET FUNCTIONS

/**
 * @brief Get RTC clock sourse
 * 
 * @param rtc: RTC
 * @retval Clock sourse: LSI_CLOCK or HSI_CLOCK
 *                    or LSE_CLOCK or HSE_CLOCK
 */
sourceClock_t
stm32rtc_get_source_clock(STM32RTC *rtc);

/**
 * @brief Get RTC subseconds
 * 
 * @param rtc: RTC
 * @retval Return the current subseconds from the RTC.
 */
ul32
stm32rtc_get_sub_seconds(STM32RTC *rtc);

/**
 * @brief Get RTC seconds
 * 
 * @param rtc: RTC
 * @retval Return the current seconds from the RTC.
 */
u8
stm32rtc_get_seconds(STM32RTC *rtc);

/**
 * @brief Get RTC minutes
 * 
 * @param rtc: RTC
 * @retval Return the current minutes from the RTC.
 */
u8
stm32rtc_get_minutes(STM32RTC *rtc);

/**
 * @brief Get RTC hours
 * 
 * @param rtc: RTC
 * @param period: Optional (default: NULL)
 *                Pointer to the current hour period set in the RTC: AM or PM
 * @retval Return the current hours from the RTC.
 */
u8
stm32rtc_get_hours
(
	STM32RTC *rtc,
	hourAM_PM_t *period
);

/**
 * @brief Get RTC time
 * 
 * @param rtc: RTC
 * @param hours: Pointer to the current hours
 * @param minutes: Pointer to the current minutes
 * @param seconds: Pointer to the current seconds
 * @param sub_seconds: Pointer to the current subseconds
 * @param period: Optional (default: NULL)
 *                Pointer to the current hour period set in the RTC: AM or PM
 * @retval None
 */
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

/**
 * @brief Get RTC day of week
 * 
 * @param rtc: RTC
 * @retval Return the current day of week from the RTC.
 */
u8
stm32rtc_get_week_day(STM32RTC *rtc);

/**
 * @brief Get RTC day
 * 
 * @param rtc: RTC
 * @retval Return the current day from the RTC.
 */
u8
stm32rtc_get_day(STM32RTC *rtc);

/**
 * @brief Get RTC month
 * 
 * @param rtc: RTC
 * @retval Return the current month from the RTC.
 */
u8
stm32rtc_get_month(STM32RTC *rtc);

/**
 * @brief Get RTC year
 * 
 * @param rtc: RTC
 * @retval Return the current year from the RTC.
 */
u8
stm32rtc_get_year(STM32RTC *rtc);

/**
 * @brief Get RTC date
 * 
 * @param rtc: RTC
 * @param wday: Pointer to the current day of week
 * @param day: Pointer to the current day
 * @param month: Pointer to the current month
 * @param year: Pointer to the current year
 * @retval None
 */
void
stm32rtc_get_date
(
	STM32RTC *rtc,
	u8 *wday,
	u8 *day,
	u8 *month,
	u8 *year
);

/**
 * @brief Get RTC alarm subseconds
 * 
 * @param rtc: RTC
 * @retval Return the current alarm subseconds
 */
ul32
stm32rtc_get_alarm_sub_seconds(STM32RTC *rtc);

/**
 * @brief Get RTC alarm seconds
 * 
 * @param rtc: RTC
 * @retval Return the current alarm seconds
 */
u8
stm32rtc_get_alarm_seconds(STM32RTC *rtc);

/**
 * @brief Get RTC alarm minutes
 * 
 * @param rtc: RTC
 * @retval Return the current alarm minute
 */
u8
stm32rtc_get_alarm_minutes(STM32RTC *rtc);

/**
 * @brief Get RTC alarm hours
 * 
 * @param rtc: RTC
 * @param period: optional (default: nullptr)
 *				  pointer to the current hour format set in the RTC: AM or PM
 * @retval Return the current alarm hours
 */
u8
stm32rtc_get_alarm_hours
(
	STM32RTC *rtc,
	hourAM_PM_t *period
);

/**
 * @brief Get RTC alarm day
 * 
 * @param rtc: RTC
 * @retval Return the current alarm day
 */
u8
stm32rtc_get_alarm_day(STM32RTC *rtc);

/**
 * @brief Get RTC alarm time and date
 * 
 * @param rtc: RTC
 * @param hours: Pointer to the current hours
 * @param minutes: Pointer to the current minutes
 * @param seconds: Pointer to the current seconds
 * @param sub_seconds: Pointer to the current subseconds
 * @param period: Optional (default: NULL)
 *                Pointer to the current hour period set in the RTC: AM or PM
 * @retval None
 */
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
);





// SET FUNCTIONS

/**
 * @brief Set RTC source clock source. By default LSI clock is selected. This
 *        method must be called before stm32rtc_begin().
 * 
 * @param rtc: RTC
 * @param source: Source Clock: LSI_CLOCK or HSI_CLOCK
 *                           or LSE_CLOCK or HSE_CLOCK
 */
void
stm32rtc_set_source_clock
(
	STM32RTC *rtc,
	sourceClock_t source
);

/**
 * @brief Set RTC subseconds
 * 
 * @param rtc: RTC
 * @param sub_seconds: 0-999
 * @retval None
 */
void
stm32rtc_set_sub_seconds
(
	STM32RTC *rtc,
	ul32 sub_seconds
);

/**
 * @brief Set RTC sseconds
 * 
 * @param rtc: RTC
 * @param seconds: 0-59
 * @retval None
 */
void
stm32rtc_set_seconds
(
	STM32RTC *rtc,
	u8 seconds
);

/**
 * @brief Set RTC minutes
 * 
 * @param rtc: RTC
 * @param minutes: 0-59
 * @retval None
 */
void
stm32rtc_set_minutes
(
	STM32RTC *rtc,
	u8 minutes
);

/**
 * @brief Set RTC hours
 * 
 * @param rtc: RTC
 * @param hours: 0-23
 * @retval None
 */
void
stm32rtc_set_hours
(
	STM32RTC *rtc,
	u8 hours
);

/**
 * @brief Set RTC hours in 12H format
 * 
 * @param rtc: RTC
 * @param hours: 0-11
 * @param period: Hour period: HOUR_AM or HOUR_PM
 * @retval None
 */
void
stm32rtc_set_hours_12
(
	STM32RTC *rtc,
	u8 hours,
	hourAM_PM_t period
);

/**
 * @brief Set RTC time
 * 
 * @param rtc: RTC
 * @param hours: 0-23
 * @param minutes: 0-59
 * @param seconds: 0-59
 * @param sub_seconds: 0-999
 * @retval None
 */
void
stm32rtc_set_time
(
	STM32RTC *rtc,
	u8 hours,
	u8 minutes,
	u8 seconds,
	ul32 sub_seconds
);

/**
 * @brief Set RTC time in 12H format
 * 
 * @param rtc: RTC
 * @param hours: 0-23
 * @param minutes: 0-59
 * @param seconds: 0-59
 * @param sub_seconds: 0-999
 * @param period: Hour period: HOUR_AM or HOUR_PM
 * @retval None
 */
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

/**
 * @brief Set RTC day of week
 * 
 * @param rtc: RTC
 * @param wday: 1-7 (Monday first)
 * @retval None
 */
void
stm32rtc_set_week_day
(
	STM32RTC *rtc,
	u8 wday
);

/**
 * @brief Set RTC day
 * 
 * @param rtc: RTC
 * @param day: 1-31
 * @retval None
 */
void
stm32rtc_set_day
(
	STM32RTC *rtc,
	u8 day
);

/**
 * @brief Set RTC month
 * 
 * @param rtc: RTC
 * @param month: 1-12
 * @retval None
 */
void
stm32rtc_set_month
(
	STM32RTC *rtc,
	u8 month
);

/**
 * @brief Set RTC year
 * 
 * @param rtc: RTC
 * @param year: 0-99
 * @retval None
 */
void
stm32rtc_set_year
(
	STM32RTC *rtc,
	u8 year
);

/**
 * @brief Set RTC date
 * 
 * @param rtc: RTC
 * @param day: 1-31
 * @param month: 1-12
 * @param year: 0-99
 * @retval None
 */
void
stm32rtc_set_date
(
	STM32RTC *rtc,
	u8 day,
	u8 month,
	u8 year
);

/**
 * @brief Set RTC date with day of week
 * 
 * @param rtc: RTC
 * @param day: 1-31
 * @param month: 1-12
 * @param year: 0-99
 * @param wday: 1-7 (Monday first)
 * @retval None
 */
void
stm32rtc_set_wdate
(
	STM32RTC *rtc,
	u8 day,
	u8 month,
	u8 year,
	u8 wday
);

/**
 * @brief Set RTC alarm subseconds
 * 
 * @param rtc: RTC
 * @param sub_seconds: 0-999
 * @retval None
 */
void
stm32rtc_set_alarm_sub_seconds
(
	STM32RTC *rtc,
	ul32 sub_seconds
);

/**
 * @brief Set RTC alarm seconds
 * 
 * @param rtc: RTC
 * @param seconds: 0-59
 * @retval None
 */
void
stm32rtc_set_alarm_seconds
(
	STM32RTC *rtc,
	u8 seconds
);

/**
 * @brief Set RTC alarm minutes
 * 
 * @param rtc: RTC
 * @param minutes: 0-59
 * @retval None
 */
void
stm32rtc_set_alarm_minutes
(
	STM32RTC *rtc,
	u8 minutes
);

/**
 * @brief Set RTC alarm hours
 * 
 * @param rtc: RTC
 * @param hours: 0-23
 * @retval None
 */
void
stm32rtc_set_alarm_hours
(
	STM32RTC *rtc,
	u8 hours
);

/**
 * @brief Set RTC alarm hours in 12H format
 * 
 * @param rtc: RTC
 * @param hours: 0-11
 * @param period: Hour period: HOUR_AM or HOUR_PM
 * @retval None
 */
void
stm32rtc_set_alarm_hours_12
(
	STM32RTC *rtc,
	u8 hours,
	hourAM_PM_t period
);

/**
 * @brief Set RTC alarm day
 * 
 * @param rtc: RTC
 * @param day: 1-31
 */
void
stm32rtc_set_alarm_day
(
	STM32RTC *rtc,
	u8 day
);

/**
 * @brief Set RTC alarm time and day
 * 
 * @param rtc: RTC
 * @param day: 1-31
 * @param hours: 0-23
 * @param minutes: 0-59
 * @param seconds: 0-59
 * @param sub_seconds: 0-999
 * @retval None
 */
void
stm32rtc_set_alarm
(
	STM32RTC *rtc,
	u8 day,
	u8 hours,
	u8 minutes,
	u8 seconds,
	ul32 sub_seconds
);

/**
 * @brief Set RTC alarm time in 12H format and day
 * 
 * @param rtc: RTC
 * @param day: 1-31
 * @param hours: 0-23
 * @param minutes: 0-59
 * @param seconds: 0-59
 * @param sub_seconds: 0-999
 * @param period: Hour period: HOUR_AM or HOUR_PM
 * @retval None
 */
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
);


// EPOCH FGUNCTIONS

/**
 * @brief Get epoch time
 * 
 * @param rtc: RTC
 * @param subseconds: Optional pointer to epoch time in subseconds
 * @retval Epoch time in seconds
 */
ul32
stm32rtc_get_epoch
(
	STM32RTC *rtc,
	ul32 *subseconds
);

/**
 * @brief Get epoch time since 1srt January 2000, 00:00:00
 * 
 * @param rtc: RTC
 * @retval Epoch time in seconds
 */
ul32
stm32rtc_get_y2k_epoch(STM32RTC *rtc);

/**
 * @brief Set RTC time from epoch time
 * 
 * @param rtc: RTC
 * @param ts: time in seconds
 * @param sub_seconds: subseconds
 * @retval None
 */
void
stm32rtc_set_epoch
(
	STM32RTC *rtc,
	ul32 ts,
	ul32 sub_seconds
);

/**
 * @brief Set epoch time since 1srt January 2000, 00:00:00
 * 
 * @param rtc: RTC
 * @param ts: Epoch Time in seconds
 */
void
stm32rtc_set_y2k_epoch
(
	STM32RTC *rtc,
	ul32 ts
);

/**
 * @brief Set  RTC alarm from epoch time
 * 
 * @param rtc: RTC
 * @param ts: time in seconds
 * @param mask: alarm mask
 * @param sub_seconds: 0-999
 * @retval None
 */
void
stm32rtc_set_alarm_epoch
(
	STM32RTC *rtc,
	ul32 ts,
	alarmMask_t mask,
	ul32 sub_seconds
);


#ifdef STM32F1xx
	/**
	 * @brief  get user asynchronous prescaler value for the current clock source.
	 * 
	 * @param rtc: RTC
	 * @param  predivA: pointer to the current Asynchronous prescaler value
	 * @param  dummy : not used (kept for compatibility reason)
	 * @retval None
	 */
	void
	stm32rtc_get_prediv
	(
		STM32RTC *rtc,
		ul32 *prediv_a,
		i16 *dummy
	);

	/**
	 * @brief  set user asynchronous prescalers value.
	 * @note   This method must be called before begin().
	 * 
	 * @param rtc: RTC
	 * @param  predivA: Asynchronous prescaler value. Reset value: RTC_AUTO_1_SECOND
	 * @param  dummy : not used (kept for compatibility reason)
	 * @retval None
	 */
	void
	stm32rtc_set_prediv
	(
		STM32RTC *rtc,
		ul32 prediv_a,
		i16 dummy
	);
#else
	/**
	 * @brief Get user (a)synchronous prescaler values if set else computed
	 *        ones for the current clock source.
	 * 
	 * @param rt: RTC
	 * @param prediv_a: Pointer to the current Asynchronous prescaler value
	 * @param prediv_s: Pointer to the current Synchronous prescaler value
	 * @retval None
	 */
	void
	stm32rtc_get_prediv
	(
		STM32RTC *rtc,
		i8 *prediv_a,
		i16* prediv_s
	);

	/**
	 * @brief Set user (a)synchronous prescalers value.
	 * @note  This method must be called before begin().
	 * 
	 * @param rtc: RTC
	 * @param prediv_a: Asynchronous prescaler value. Reset value: -1
	 * @param prediv_s: Synchronous prescaler value. Reset value: -1
	 * @retval None
	 */
	void
	stm32rtc_set_prediv
	(
		STM32RTC *rtc,
		i8 prediv_a,
		i16 prediv_s
	);
#endif /* STM32F1xx */

bool
stm32rtc_is_configured(STM32RTC *rtc);

bool
stm32rtc_is_alarm_enabled(STM32RTC *rtc);

bool
stm32rtc_is_time_set(STM32RTC *rtc);



// SYNCHRONISATION FUNCTIONS

/**
 * @brief Synchronise the time from the current RTC one
 * 
 * @param rtc: RTC
 * @retval None
 */
void
stm32rtc_sync_time(STM32RTC *rtc);

/**
 * @brief Synchronise the date from the current RTC one
 * 
 * @param rtc: RTC
 * @retval None
 */
void
stm32rtc_sync_date(STM32RTC *rtc);

/**
 * @brief Synchronise the alarm time from the current RTC one
 * 
 * @param rtc: RTC
 * @retval None
 */
void
stm32rtc_sync_alarm_time(STM32RTC *rtc);

#endif /* STM32_RTC_H_ */

#endif /* __cplusplus */