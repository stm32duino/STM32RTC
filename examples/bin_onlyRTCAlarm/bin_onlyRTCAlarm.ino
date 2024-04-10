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

#include <STM32RTCMbed.h>

/* Get the rtc object */
STM32RTC& rtc = STM32RTC::getInstance();

uint32_t timeout;

void setup()
{
  Serial.begin(115200);
  while (!Serial) ;

  delay(2000);

  // Select RTC clock source: LSI_CLOCK, LSE_CLOCK or HSE_CLOCK.
  rtc.setClockSource(STM32RTC::LSE_CLOCK);

  /* Configure the RTC mode */
  rtc.setBinaryMode(STM32RTC::MODE_BIN);

  /* in BIN mode time and Date register are not used, only the subsecond register for milisseconds */
  rtc.begin(true, STM32RTC::HOUR_24);

  /* wait for a while */
  delay(300);

  /* subsecond expressed in milliseconds */
  Serial.print("Start at ");
  Serial.print(rtc.getSubSeconds());
  Serial.println(" ms");

  /* Attach the callback function before enabling Interrupt */
  rtc.attachInterrupt(alarmAMatch);

  /* Program the AlarmA in 12 seconds */
  rtc.setAlarmTime(0, 0, 0, 12000);
  rtc.enableAlarm(rtc.MATCH_SUBSEC);
  Serial.print("Set Alarm A in 12s (at ");
  Serial.print(rtc.getSubSeconds());
  Serial.println(" ms)");

#ifdef RTC_ALARM_B
  /* Program ALARM B in 600ms ms from now (keep timeout < 1000ms) */
  timeout = rtc.getSubSeconds() + 600;

  rtc.attachInterrupt(alarmBMatch, STM32RTC::ALARM_B);
  rtc.setAlarmSubSeconds(timeout, STM32RTC::ALARM_B);
  rtc.enableAlarm(rtc.MATCH_SUBSEC, STM32RTC::ALARM_B);
  Serial.print("Set Alarm B (in 600 ms) at ");
  Serial.print(rtc.getSubSeconds());
  Serial.println(" ms");
#endif /* RTC_ALARM_B */

  Serial.println("Wait for Alarm A or B");
}

void loop()
{

}

void alarmAMatch(void *data)
{
  UNUSED(data);
  rtc.disableAlarm(STM32RTC::ALARM_A);
  Serial.print("Alarm A Match at ");
  Serial.print(rtc.getSubSeconds());
  Serial.println(" ms");
}

#ifdef RTC_ALARM_B
void alarmBMatch(void *data)
{
  UNUSED(data);
  rtc.disableAlarm(STM32RTC::ALARM_B); /* Else it will trig again */
  Serial.print("Alarm B Match at ");
  Serial.print(rtc.getSubSeconds());
  Serial.println(" ms");
}
#endif /* RTC_ALARM_B */
