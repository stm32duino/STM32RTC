/*
  change RTC mode from BCD --> MIX --> BIN

  This sketch shows that changing the RTC mode does not affect the calendar
  Configure the RTC, set an alarm, on alarm match change the RTC mode.

  Creation 01 march 2024
  by Francois Ramu for STMicroelectronics

  This example code is in the public domain.

  https://github.com/stm32duino/STM32RTC
*/

#include <STM32RTCMbed.h>

/* Get the rtc object */
STM32RTC& rtc = STM32RTC::getInstance();
bool rtc_mode_changed;

void setup()
{
  Serial.begin(115200);
  while (!Serial) ;

  delay(2000);

  // Select RTC clock source: LSI_CLOCK, LSE_CLOCK or HSE_CLOCK.
  rtc.setClockSource(STM32RTC::LSE_CLOCK);

  /* Configure the RTC mode */
  rtc.setBinaryMode(STM32RTC::MODE_BCD);
  rtc_mode_changed = true;

  /* RTC mode is in BCD mode by default */
  rtc.begin(true, STM32RTC::HOUR_24);

  /* wait for a while */
  delay(500);

  // Set the calendar
  rtc.setDate(01, 03, 24);
  rtc.setTime(14, 02, 58);

  // Program ALARM A to change the RTC mode (set then enable)
  rtc.attachInterrupt(alarmMatch);
  rtc.setAlarmSeconds(5, STM32RTC::ALARM_A);
  rtc.enableAlarm(rtc.MATCH_SS, STM32RTC::ALARM_A);

  Serial.println("Wait for Alarm A");
}

void loop()
{
  /* Reports the RTC  mode when it has been changed */
  if (rtc_mode_changed) {
    if (rtc.getBinaryMode() == STM32RTC::MODE_BCD) {
      Serial.print("RTC mode is MODE_BCD at ");
    } else if (rtc.getBinaryMode() == STM32RTC::MODE_MIX) {
      Serial.print("RTC mode is MODE_MIX at ");
    } else {
      Serial.print("RTC mode is MODE_BIN at ");
    }

    Serial.print(rtc.getDay());
    Serial.print("/");
    Serial.print(rtc.getMonth());
    Serial.print("/");
    Serial.print(rtc.getYear());
    Serial.print(" - ");
    Serial.print(rtc.getHours());
    Serial.print(":");
    Serial.print(rtc.getMinutes());
    Serial.print(":");
    Serial.println(rtc.getSeconds());

    rtc_mode_changed = false;
  }
}

void alarmMatch(void *data)
{
  UNUSED(data);
  Serial.print("Alarm A Match! : change RTC mode at ");
  Serial.print(rtc.getHours());
  Serial.print(":");
  Serial.print(rtc.getMinutes());
  Serial.print(":");
  Serial.println(rtc.getSeconds());

   /* Configure the new RTC mode */
  if (rtc.getBinaryMode() == STM32RTC::MODE_BCD) {
    rtc.setBinaryMode(STM32RTC::MODE_MIX);
  } else if (rtc.getBinaryMode() == STM32RTC::MODE_MIX) {
    rtc.setBinaryMode(STM32RTC::MODE_BIN);
  } else {
    rtc.setBinaryMode(STM32RTC::MODE_BCD);
  }
  rtc_mode_changed = true;
}
