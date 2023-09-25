/*
  mode BINary only RTC alarm

  This sketch shows how to configure the alarm A & B of the RTC in BIN mode

  Creation 12 Dec 2017
  by Wi6Labs
  Modified 03 Jul 2020
  by Frederic Pillon for STMicroelectronics
  Modified 03 sept 2023
  by Francois Ramu for STMicroelectronics

  This example code is in the public domain.

  https://github.com/stm32duino/STM32RTC
*/

#include <STM32RTC.h>

/* Get the rtc object */
STM32RTC& rtc = STM32RTC::getInstance();

uint32_t timeout;

void setup()
{
  Serial.begin(115200);

  // Select RTC clock source: LSI_CLOCK, LSE_CLOCK or HSE_CLOCK.
  rtc.setClockSource(STM32RTC::LSE_CLOCK);

  /* Configure the RTC mode */
  rtc.setBinaryMode(STM32RTC::MODE_BIN);

  /* in BIN mode time and Date register are not used, only the subsecond register for milisseconds */
  rtc.begin(true, STM32RTC::HOUR_24);

  /* wait for a while */
  delay(300);

  /* subsecond expressed in milliseconds */
  Serial.printf("Start at %d ms \r\n", rtc.getSubSeconds());

  /* Attach the callback function before enabling Interrupt */
  rtc.attachInterrupt(alarmAMatch);

  /* Program the AlarmA in 12 seconds */
  rtc.setAlarmTime(0, 0, 0, 12000);
  rtc.enableAlarm(rtc.MATCH_SUBSEC);
  Serial.printf("Set Alarm A in 12s (at %d ms)\r\n", rtc.getAlarmSubSeconds());

#ifdef RTC_ALARM_B
  /* Program ALARM B in 600ms ms from now (keep timeout < 1000ms) */
  timeout = rtc.getSubSeconds() + 600;

  rtc.attachInterrupt(alarmBMatch, STM32RTC::ALARM_B);
  rtc.setAlarmSubSeconds(timeout, STM32RTC::ALARM_B);
  rtc.enableAlarm(rtc.MATCH_SUBSEC, STM32RTC::ALARM_B);
  Serial.printf("Set Alarm B (in %d ms) at %d ms\r\n", 600,
          rtc.getAlarmSubSeconds(STM32RTC::ALARM_B));
#endif /* RTC_ALARM_B */

}

void loop()
{

}

void alarmAMatch(void *data)
{
  UNUSED(data);
  rtc.disableAlarm(STM32RTC::ALARM_A);
  Serial.printf("Alarm A Match at %d ms \r\n", rtc.getSubSeconds());
}

#ifdef RTC_ALARM_B
void alarmBMatch(void *data)
{
  UNUSED(data);
  rtc.disableAlarm(STM32RTC::ALARM_B); /* Else it will trig again */
  Serial.printf("Alarm B Match at %d ms\r\n", rtc.getSubSeconds());
}
#endif /* RTC_ALARM_B */

