/*
  Epoch

  This sketch shows how to configure the RTC alarm

  Creation 12 Dec 2017
  by Wi6Labs
  Modified 03 Jul 2020
  by Frederic Pillon for STMicroelectronics

  This example code is in the public domain.

  https://github.com/stm32duino/STM32RTC
*/

#include <STM32RTCMbed.h>

/* Get the rtc object */
STM32RTC& rtc = STM32RTC::getInstance();

/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 0;
const byte hours = 16;

/* Change these values to set the current initial date */
const byte day = 25;
const byte month = 9;
const byte year = 15;

void setup()
{
  Serial.begin(115200);
  while (!Serial) ;

  delay(2000);

  // Select RTC clock source: LSI_CLOCK, LSE_CLOCK or HSE_CLOCK.
  // By default the LSI is selected as source.
  // rtc.setClockSource(STM32RTC::LSE_CLOCK);

  rtc.begin(); // initialize RTC 24H format

  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);

  rtc.attachInterrupt(alarmMatch);
  rtc.setAlarmDay(day);
  rtc.setAlarmTime(16, 0, 10, 123);
  rtc.enableAlarm(rtc.MATCH_DHHMMSS);

  Serial.println("Wait for Alarm A");
}

void loop()
{
  static byte seconds = 0;
  static byte minutes = 0;
  static byte hours = 0;
  static uint32_t subSeconds = 0;
  static STM32RTC::AM_PM period = STM32RTC::AM;

  rtc.getTime(&hours, &minutes, &seconds, &subSeconds, &period);
  // Print current date & time
  Serial.print(rtc.getDay());
  Serial.print("/");
  Serial.print(rtc.getMonth());
  Serial.print("/");
  Serial.print(rtc.getYear());
  Serial.print(" ");
  Serial.print(hours);
  Serial.print(":");
  Serial.print(minutes);
  Serial.print(":");
  Serial.print(seconds);
  Serial.print(".");
  Serial.println(subSeconds);
  // Print current alarm configuration
  Serial.print("Alarm A: ");
  Serial.print(rtc.getAlarmDay());
  Serial.print(" ");
  Serial.print(rtc.getAlarmHours());
  Serial.print(":");
  Serial.print(rtc.getAlarmMinutes());
  Serial.print(":");
  Serial.print(rtc.getAlarmSeconds());
  Serial.print(".");
  Serial.println(rtc.getAlarmSubSeconds());

  delay(1000);
}

void alarmMatch(void *data)
{
  UNUSED(data);
  Serial.println("Alarm Match!");
}
