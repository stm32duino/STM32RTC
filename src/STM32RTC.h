/**
  ******************************************************************************
  * @file    STM32RTC.h
  * @author  WI6LABS
  * @version V1.0.0
  * @date    12-December-2017
  * @brief   Provides a RTC interface for Arduino
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
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

#ifndef __STM32_RTC_H
#define __STM32_RTC_H

#include "Arduino.h"

// Check if RTC HAL enable in variants/board_name/stm32yzxx_hal_conf.h
#ifndef HAL_RTC_MODULE_ENABLED
#error "RTC configuration is missing. Check flag HAL_RTC_MODULE_ENABLED in variants/board_name/stm32yzxx_hal_conf.h"
#endif

typedef void(*voidFuncPtr)(void *);

// Hour format: 12 or 24 hours
typedef enum {
  HOUR_12,
  HOUR_24
} RTCHourFormats_t;

//Time AM/PM definition
typedef enum {
  HOUR12_AM,
  HOUR12_PM
} Hour12_AM_PM_t;

// Clock source selection
typedef enum {
  RTC_LSI_CLOCK,
  RTC_LSE_CLOCK,
  RTC_HSE_CLOCK
} RTC_sourceClock_t;

#define IS_CLOCK_SOURCE(SRC) (((SRC) == RTC_LSI_CLOCK) || ((SRC) == RTC_LSE_CLOCK) ||\
                              ((SRC) == RTC_HSE_CLOCK))

class STM32RTC {
public:

  enum Alarm_Match: uint8_t
  {
    MATCH_OFF          = 0, // Never
    MATCH_SS           = 1, // Every Minute
    MATCH_MMSS         = 2, // Every Hour
    MATCH_HHMMSS       = 3, // Every Day
    MATCH_DHHMMSS      = 4, // Every Month

    /* NOTE: STM32 RTC can't assign a month or a year to an alarm. Those enum
    are kept for compatibility but are ignored inside enableAlarm(). */
    MATCH_MMDDHHMMSS   = 5,
    MATCH_YYMMDDHHMMSS = 6
  };

  STM32RTC();

  void begin(bool resetTime, RTCHourFormats_t format = HOUR_24);
  void begin(RTCHourFormats_t format = HOUR_24);

  void setClockSource(RTC_sourceClock_t source);

  void enableAlarm(Alarm_Match match);
  void disableAlarm(void);

  void attachInterrupt(voidFuncPtr callback, void *data = NULL);
  void detachInterrupt(void);

  // Kept for compatibility: use STM32LowPower library.
  void standbyMode();

  /* Get Functions */

  uint32_t getSubSeconds(void);
  uint8_t getSeconds(void);
  uint8_t getMinutes(void);
  uint8_t getHours(void);
  uint8_t getHours(Hour12_AM_PM_t *format);

  uint8_t getWeekDay(void);
  uint8_t getDay(void);
  uint8_t getMonth(void);
  uint8_t getYear(void);

  uint32_t getAlarmSubSeconds(void);
  uint8_t getAlarmSeconds(void);
  uint8_t getAlarmMinutes(void);
  uint8_t getAlarmHours(void);
  uint8_t getAlarmHours(Hour12_AM_PM_t *format);

  uint8_t getAlarmDay(void);

  // Kept for compatibility with Arduino RTCZero library.
  uint8_t getAlarmMonth();
  uint8_t getAlarmYear();

  /* Set Functions */

  void setSubSeconds(uint32_t subSeconds);
  void setSeconds(uint8_t seconds);
  void setMinutes(uint8_t minutes);
  void setHours(uint8_t hours);
  void setHours(uint8_t hours, Hour12_AM_PM_t format);
  void setTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
  void setTime(uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t subSeconds, Hour12_AM_PM_t format);

  void setWeekDay(uint8_t weekDay);
  void setDay(uint8_t day);
  void setMonth(uint8_t month);
  void setYear(uint8_t year);
  void setDate(uint8_t day, uint8_t month, uint8_t year);
  void setDate(uint8_t weekDay, uint8_t day, uint8_t month, uint8_t year);

  void setAlarmSeconds(uint8_t seconds);
  void setAlarmMinutes(uint8_t minutes);
  void setAlarmHours(uint8_t hours);
  void setAlarmHours(uint8_t hours, Hour12_AM_PM_t format);
  void setAlarmTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
  void setAlarmTime(uint8_t hours, uint8_t minutes, uint8_t seconds, Hour12_AM_PM_t format);

  void setAlarmDay(uint8_t day);

  // Kept for compatibility with Arduino RTCZero library.
  void setAlarmMonth(uint8_t month);
  void setAlarmYear(uint8_t year);
  void setAlarmDate(uint8_t day, uint8_t month, uint8_t year);

  /* Epoch Functions */

  uint32_t getEpoch(void);
  uint32_t getY2kEpoch(void);
  void setEpoch(uint32_t ts);
  void setY2kEpoch(uint32_t ts);
  void setAlarmEpoch(uint32_t ts);

  bool isConfigured(void) {
    return _configured;
  }

private:
  static bool _configured;

  Hour12_AM_PM_t _hoursFormat;
  uint8_t     _hours;
  uint8_t     _minutes;
  uint8_t     _seconds;
  uint32_t    _subSeconds;
  uint8_t     _year;
  uint8_t     _month;
  uint8_t     _date;
  uint8_t     _day;

  uint8_t     _alarmDate;
  uint8_t     _alarmHours;
  uint8_t     _alarmMinutes;
  uint8_t     _alarmSeconds;
  uint32_t    _alarmSubSeconds;
  Hour12_AM_PM_t _alarmFormat;

  RTC_sourceClock_t _clockSource;
};

#endif // __STM32_RTC_H
