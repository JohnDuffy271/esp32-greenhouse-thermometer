#include "RTC.h"
#include <DS3231.h>

static DS3231 rtc;          // northernwidget DS3231
static RTCDateTime dt;      // struct from the library

bool RTC::_initialized = false;

bool RTC::begin(uint8_t sdaPin, uint8_t sclPin) {
  if (_initialized) return true;

  Wire.begin(sdaPin, sclPin);
  Serial.printf("[RTC] I2C started (SDA=%u, SCL=%u)\n", sdaPin, sclPin);

  // DS3231 lib doesn't always "probe", so do a basic I2C check
  Wire.beginTransmission(0x68);
  uint8_t err = Wire.endTransmission();
  if (err != 0) {
    Serial.printf("[RTC] DS3231 not detected at 0x68 (err=%u)\n", err);
    return false;
  }

  _initialized = true;
  Serial.println("[RTC] DS3231 detected OK");
  return true;
}

bool RTC::getTime(time_t &t) {
  if (!_initialized) {
    Serial.println("[RTC] getTime() called before begin()");
    return false;
  }

  dt = rtc.getDateTime();

  // Build a tm struct
  struct tm tmNow = {};
  tmNow.tm_year = dt.year - 1900;
  tmNow.tm_mon  = dt.month - 1;
  tmNow.tm_mday = dt.day;
  tmNow.tm_hour = dt.hour;
  tmNow.tm_min  = dt.minute;
  tmNow.tm_sec  = dt.second;
  tmNow.tm_isdst = 0;

  // mktime() treats tm as local time; for greenhouse logging we can treat as UTC-ish.
  // If you care about BST/GMT properly, we can add timezone handling later.
  t = mktime(&tmNow);

  if (t <= 0) {
    Serial.println("[RTC] Failed to convert DS3231 time to epoch");
    return false;
  }

  Serial.printf("[RTC] %04u-%02u-%02u %02u:%02u:%02u (epoch=%lu)\n",
                dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second,
                (unsigned long)t);

  return true;
}
