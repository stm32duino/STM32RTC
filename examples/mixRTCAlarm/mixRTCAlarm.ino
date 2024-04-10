/*
  mode Mix RTC alarm

  This sketch shows how to configure the alarm A & B (if exists)
  of the RTC in MIX or BCD (BINARY none) mode

  Creation 12 Dec 2017
  by Wi6Labs
  Modified 03 Jul 2020
  by Frederic Pillon for STMicroelectronics
  Modified 03 Jul 2023
  by Francois Ramu for STMicroelectronics

  This example code is in the public domain.

  https://github.com/stm32duino/STM32RTC
*/

#include <STM32RTCMbed.h>

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
  while (!Serial) ;

  delay(2000);

  // Select RTC clock source: LSI_CLOCK, LSE_CLOCK or HSE_CLOCK.
  rtc.setClockSource(STM32RTC::LSE_CLOCK);

  /* Configure the RTC mode : STM32RTC::MODE_MIX or STM32RTC::MODE_BCD */
  rtc.setBinaryMode(STM32RTC::MODE_BCD);

  rtc.begin(true, STM32RTC::HOUR_24);

  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);

  /* wait for a while */
  delay(300);

  Serial.print("Start at ");
  Serial.print(rtc.getHours());
  Serial.print(":");
  Serial.print(rtc.getMinutes());
  Serial.print(":");
  Serial.print(rtc.getSeconds());
  Serial.print(".");
  Serial.println(rtc.getSubSeconds());

  /* Attach the callback function before enabling Interrupt */
  rtc.attachInterrupt(alarmAMatch);

  /* Program the AlarmA in 12 seconds */
  rtc.setAlarmDay(day);
  rtc.setAlarmTime(hours, minutes, seconds + 12);
  rtc.enableAlarm(rtc.MATCH_DHHMMSS);
  Serial.print("Set Alarm A in 12s (at ");
  Serial.print(rtc.getAlarmHours());
  Serial.print(":");
  Serial.print(rtc.getAlarmMinutes());
  Serial.print(":");
  Serial.print(rtc.getAlarmSeconds());
  Serial.println(")");

#ifdef RTC_ALARM_B
  /* Program ALARM B in 600ms ms from now (keep timeout < 1000ms) */
  timeout = rtc.getSubSeconds() + 600;

  rtc.attachInterrupt(alarmBMatch, STM32RTC::ALARM_B);
  rtc.setAlarmSubSeconds(timeout, STM32RTC::ALARM_B);
  rtc.enableAlarm(rtc.MATCH_SUBSEC, STM32RTC::ALARM_B);
  Serial.print("Set Alarm B (in 600 ms) at ");
  Serial.print(rtc.getAlarmSubSeconds(STM32RTC::ALARM_B));
  Serial.println(" ms");
#endif /* RTC_ALARM_B */

  Serial.println("Wait for Alarm A or B");
}

void loop()
{
  /* Just wait for Alarm */
}

void alarmAMatch(void *data)
{
  UNUSED(data);
  rtc.disableAlarm(STM32RTC::ALARM_A);
  Serial.print("Alarm A Match at ");
  Serial.print(rtc.getAlarmHours());
  Serial.print(":");
  Serial.print(rtc.getAlarmMinutes());
  Serial.print(":");
  Serial.println(rtc.getAlarmSeconds());
}

#ifdef RTC_ALARM_B
void alarmBMatch(void *data)
{
  UNUSED(data);
  rtc.disableAlarm(STM32RTC::ALARM_B);
  Serial.print("Alarm B Match at ");
  Serial.print(rtc.getSubSeconds());
  Serial.println(" ms");
}
#endif /* RTC_ALARM_B */
