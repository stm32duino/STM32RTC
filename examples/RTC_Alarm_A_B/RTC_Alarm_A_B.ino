/*
  SimpleRTC

  This sketch shows how to configure the RTC 
  for two separate interrupts

  This example code is in the public domain.

  https://github.com/stm32duino/STM32RTC
*/

#define SERIAL_ENABLED
#ifdef __AVR__
#define TheSerial Serial
#else
#include "HardwareSerial.h"
//                     RX   TX
HardwareSerial Serial2(PA3, PA2);
#define TheSerial Serial2
#endif

#include <STM32RTC.h>

/* Get the rtc object */
STM32RTC& rtc = STM32RTC::getInstance();

const byte alarm_a_second = 30;
const byte alarm_b_second = 12;

void setup()
{
  TheSerial.begin(38400);

  rtc.begin();
  rtc.setAlarmType(RTC_ALARM_A);
  rtc.attachInterrupt(alarmACallback, NULL);
  rtc.setAlarmEpoch(rtc.getEpoch() + alarm_a_second);
  
  rtc.setAlarmType(RTC_ALARM_B);
  rtc.attachInterrupt(alarmBCallback, NULL);
  rtc.setAlarmEpoch(rtc.getEpoch() + alarm_b_second);
}

void loop()
{
}

void alarmACallback(void *data)
{
  TheSerial.println("Alarm A called");
}

void alarmBCallback(void *data)
{
  TheSerial.println("Alarm A called");
}
