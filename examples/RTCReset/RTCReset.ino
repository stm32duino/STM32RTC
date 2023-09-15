/*
  RTCReset

  This sketch allows to test STM32RTC after a software reset or power off
  with VBat. Including Alarm A and B management.

  Creation 17 jan 2023
  by Frederic Pillon for STMicroelectronics

  This example code is in the public domain.

  https://github.com/stm32duino/STM32RTC

*/
#include <STM32RTC.h>

/* Get the rtc object */
STM32RTC& rtc = STM32RTC::getInstance();

/* Change these values to set the current initial time

   format: date: "Dec 31 2022" and time: "23:59:56"
   by default use built date and time
*/
static const char* mydate = __DATE__;
static const char* mytime = __TIME__;
//static const char* mydate = "Dec 31 2022";
//static const char* mytime = "23:59:56";

/* Declare it volatile since it's incremented inside an interrupt */
volatile int alarmMatch_counter = 0;
volatile int alarmMatchB_counter = 0;

typedef struct {
  uint32_t next;
  bool alarm_a;
} cb_data_t;

static cb_data_t atime = { 2222, true };
#ifdef RTC_ALARM_B
static cb_data_t btime = { 3333, false };
#endif
static byte seconds = 0;
static byte minutes = 0;
static byte hours = 0;
static uint32_t subSeconds = 0;

static byte weekDay = 1;
static byte day = 0;
static byte month = 0;
static byte year = 0;
static STM32RTC::Hour_Format hourFormat = STM32RTC::HOUR_24;
static STM32RTC::AM_PM period = STM32RTC::AM;

#ifndef USER_BTN
#define USER_BTN PA0
#endif

static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}

// sample input: date = "Dec 26 2009", time = "12:34:56"
void initDateTime(void) {
  Serial.printf("Build date & time %s, %s\n", mydate, mytime);

  year = conv2d(mydate + 9);
  // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
  switch (mydate[0]) {
    case 'J': month = (mydate[1] == 'a') ? 1 : ((mydate[2] == 'n') ? 6 : 7); break;
    case 'F': month = 2; break;
    case 'A': month = mydate[2] == 'r' ? 4 : 8; break;
    case 'M': month = mydate[2] == 'r' ? 3 : 5; break;
    case 'S': month = 9; break;
    case 'O': month = 10; break;
    case 'N': month = 11; break;
    case 'D': month = 12; break;
  }
  day = conv2d(mydate + 4);
  hours = conv2d(mytime);
  if (hourFormat == rtc.HOUR_12) {
    period = hours >= 12 ? rtc.PM : rtc.AM;
    hours = hours >= 13 ? hours - 12 : (hours < 1 ? hours + 12 : hours);
  }
  minutes = conv2d(mytime + 3);
  seconds = conv2d(mytime + 6);
}

void setup() {
  pinMode(USER_BTN, INPUT_PULLUP);
  int32_t default_state = digitalRead(USER_BTN);
  Serial.begin(115200);
  while (!Serial)
    ;
  // Wait user input to start
  while (digitalRead(USER_BTN) == default_state)
    ;
  // Convenient function to init date and time variables
  initDateTime();

  // Select RTC clock source: LSI_CLOCK, LSE_CLOCK or HSE_CLOCK.
  // By default the LSI is selected as source.
  // rtc.setClockSource(STM32RTC::LSE_CLOCK);

  // In any case attach a callback to the RTC alarm interrupt.
  rtc.attachInterrupt(alarmMatch, &atime);
#ifdef RTC_ALARM_B
  rtc.attachInterrupt(alarmMatch, &btime, STM32RTC::ALARM_B);
#endif
  rtc.begin();  // Initialize RTC 24H format
  if (!rtc.isTimeSet()) {
    Serial.printf("RTC time not set\n Set it.\n");
    // Set the time
    rtc.setTime(hours, minutes, seconds);
    rtc.setDate(weekDay, day, month, year);
    // ALARM_A (default argument)
    rtc.setAlarmDay(day);
    rtc.setAlarmTime(hours, minutes, seconds + 5, 567);
    rtc.enableAlarm(rtc.MATCH_DHHMMSS);
#ifdef RTC_ALARM_B
    // ALARM_B
    rtc.setAlarmDay(day, STM32RTC::ALARM_B);
    rtc.setAlarmTime(hours, minutes, seconds + 5, 567, STM32RTC::ALARM_B);
    rtc.enableAlarm(rtc.MATCH_DHHMMSS, STM32RTC::ALARM_B);
#endif
  } else {
    // RTC already initialized
    time_t epoc, alarm_epoc;
    rtc.getTime(&hours, &minutes, &seconds, &subSeconds, &period);
    year = rtc.getYear();
    month = rtc.getMonth();
    day = rtc.getDay();
    if (rtc.isAlarmEnabled()) {
      rtc.enableAlarm(rtc.MATCH_DHHMMSS);
      alarm_epoc = rtc.getAlarmEpoch();
      epoc = rtc.getEpoch();
      if (difftime(alarm_epoc, epoc) <= 0) {
        Serial.printf("Alarm A was enabled and expired, force callback call\n");
        alarmMatch(&atime);
      } else {
        Serial.printf("Alarm A was enabled and restored\n");
      }
    }
#ifdef RTC_ALARM_B
    // ALARM_B
    if (rtc.isAlarmEnabled(STM32RTC::ALARM_B)) {
      rtc.enableAlarm(rtc.MATCH_DHHMMSS, STM32RTC::ALARM_B);
      alarm_epoc = rtc.getAlarmEpoch(STM32RTC::ALARM_B);
      epoc = rtc.getEpoch();
      if (difftime(alarm_epoc, epoc) <= 0) {
        Serial.printf("Alarm B was enabled and expired, force callback call\n");
        alarmMatch(&btime);
      } else {
        Serial.printf("Alarm B was enabled and restored\n");
      }
    }
#endif
    Serial.printf("RTC time already set\n");
  }
  // For STM32F1xx series, alarm is always disabled after a reset.
  bool alarmA = rtc.isAlarmEnabled(STM32RTC::ALARM_A);
  Serial.printf("Alarm A enable status: %s\n", (alarmA) ? "True" : "False");
  if (!alarmA) {
    rtc.setAlarmDay(day);
    rtc.setAlarmTime(hours, minutes, seconds + 5, 567);
    rtc.enableAlarm(rtc.MATCH_DHHMMSS);
  }
#ifdef RTC_ALARM_B
  bool alarmB = rtc.isAlarmEnabled(STM32RTC::ALARM_B);
  Serial.printf("Alarm B enable status: %s\n", (alarmB) ? "True" : "False");
  if (!alarmB) {
    rtc.setAlarmDay(day, STM32RTC::ALARM_B);
    rtc.setAlarmTime(hours, minutes, seconds + 5, 567, STM32RTC::ALARM_B);
    rtc.enableAlarm(rtc.MATCH_DHHMMSS, STM32RTC::ALARM_B);
  }
#else
  Serial.println("Alarm B not available.");
#endif
}

void loop() {
  rtc.getTime(&hours, &minutes, &seconds, &subSeconds, &period);
  // Print current date & time
  Serial.printf("\n%02d/%02d/%02d %02d:%02d:%02d.%03d\n", rtc.getDay(), rtc.getMonth(), rtc.getYear(), hours, minutes, seconds, subSeconds);
  // Print current alarm configuration
  Serial.printf("Alarm A: %02d %02d:%02d:%02d.%03d\n", rtc.getAlarmDay(), rtc.getAlarmHours(), rtc.getAlarmMinutes(), rtc.getAlarmSeconds(), rtc.getAlarmSubSeconds());
#ifdef RTC_ALARM_B
  Serial.printf("Alarm B: %02d %02d:%02d:%02d.%03d\n", rtc.getAlarmDay(STM32RTC::ALARM_B), rtc.getAlarmHours(STM32RTC::ALARM_B), rtc.getAlarmMinutes(STM32RTC::ALARM_B), rtc.getAlarmSeconds(STM32RTC::ALARM_B), rtc.getAlarmSubSeconds(STM32RTC::ALARM_B));
#endif
  delay(1000);
}

void alarmMatch(void* data) {
  time_t epoc;
  uint32_t epoc_ms;
  uint32_t sec = 0;
  uint32_t _millis = 1000;
  cb_data_t cbdata = { .next = 1000, .alarm_a = true };
  if (data != NULL) {
    cbdata.next = ((cb_data_t*)data)->next;
    cbdata.alarm_a = ((cb_data_t*)data)->alarm_a;
    _millis = cbdata.next;
  }

  sec = _millis / 1000;
#if !defined(RTC_SSR_SS)
  // Minimum is 1 second
  if (sec == 0) {
    sec = 1;
  }
  epoc = rtc.getEpoch(&epoc_ms);
#else
  _millis = _millis % 1000;
  epoc = rtc.getEpoch(&epoc_ms);

  // Update epoch_ms - might need to add a second to epoch
  epoc_ms += _millis;
  if (epoc_ms >= 1000) {
    sec++;
    epoc_ms -= 1000;
  }
#endif
  if (cbdata.alarm_a) {
    Serial.printf("\t\t\tAlarm A Match %i\n", ++alarmMatch_counter);
    rtc.setAlarmEpoch(epoc + sec, STM32RTC::MATCH_SS, epoc_ms);
  }
#ifdef RTC_ALARM_B
  else {
    Serial.printf("\t\t\tAlarm B Match %i\n", ++alarmMatchB_counter);
    rtc.setAlarmEpoch(epoc + sec, STM32RTC::MATCH_SS, epoc_ms, STM32RTC::ALARM_B);
  }
#endif
}
