/*
  mode Mix RTC alarm

  This sketch shows how to configure the alarm A & B of the RTC in MIX mode

  Creation 12 Dec 2017
  by Wi6Labs
  Modified 03 Jul 2020
  by Frederic Pillon for STMicroelectronics
  Modified 03 Jul 2023
  by Francois Ramu for STMicroelectronics

  This example code is in the public domain.

  https://github.com/stm32duino/STM32RTC
*/

#include <STM32RTC.h>

/* Get the rtc object */
STM32RTC& rtc = STM32RTC::getInstance();

/* Change these values to set the current initial time */
const byte seconds = 06;
const byte minutes = 22;
const byte hours = 16;

/* Change these values to set the current initial date */
const byte day = 25;
const byte month = 6;
const byte year = 23;

uint32_t timeout;

void setup()
{
  Serial.begin(115200);

  // Select RTC clock source: LSI_CLOCK, LSE_CLOCK or HSE_CLOCK.
  rtc.setClockSource(STM32RTC::LSE_CLOCK);

  /* Configure the RTC mode : STM32RTC::MODE_MIX or STM32RTC::MODE_BCD */
  rtc.setBinaryMode(STM32RTC::MODE_MIX);

  rtc.begin(true, STM32RTC::HOUR_24);

  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);

  /* wait for a while */
  delay(200);

  Serial.printf("Start at %02d:%02d:%02d.%03d\r\n",
          rtc.getHours(), rtc.getMinutes(), rtc.getSeconds(), rtc.getSubSeconds());

  /* Attach the callback function before enabling Interrupt */
  rtc.attachInterrupt(alarmAMatch);

  /* Program the AlarmA in a 12 seconds */
  rtc.setAlarmDay(day);
  rtc.setAlarmTime(hours, minutes, seconds + 12);
  rtc.enableAlarm(rtc.MATCH_DHHMMSS);
  Serial.printf("Set Alarm A in 12s (at %02d:%02d:%02d)\r\n",
          rtc.getAlarmHours(), rtc.getAlarmMinutes(), rtc.getAlarmSeconds());

#ifdef RTC_ALARM_B
  /* Program ALARM B in 400ms ms from now (keep timeout < 1000ms) */
  timeout = rtc.getSubSeconds() + 400;

  rtc.attachInterrupt(alarmBMatch, STM32RTC::ALARM_B);
  rtc.setAlarmSubSeconds(timeout, STM32RTC::ALARM_B);
  rtc.enableAlarm(rtc.MATCH_SUBSEC, STM32RTC::ALARM_B);
  Serial.printf("Set Alarm B (in %d ms) at %d ms\r\n", 400,
  rtc.getAlarmSubSeconds(STM32RTC::ALARM_B));
#endif

}

void loop()
{

}

void alarmAMatch(void *data)
{
  UNUSED(data);
  rtc.disableAlarm(STM32RTC::ALARM_A);
  Serial.printf("Alarm A Match at %02d:%02d:%02d\r\n",
          rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
}

void alarmBMatch(void *data)
{
  UNUSED(data);
  rtc.disableAlarm(STM32RTC::ALARM_B); /* Else it will trig again */
  Serial.printf("Alarm B Match at %d ms\r\n", rtc.getSubSeconds());
}


