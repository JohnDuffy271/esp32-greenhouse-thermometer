#include "RTC.h"
#include <RTClib.h>

static RTC_DS3231 rtc;

bool RTC::initialized = false;

bool RTC::begin() {
  if (initialized) {
    return true;
  }

  Wire.begin(21, 22);
  Serial.println("[RTC] I2C initialized (SDA=21, SCL=22)");

  if (!rtc.begin()) {
    Serial.println("[RTC] Error: DS3231 not found on I2C");
    return false;
  }

  // Optional: detect lost power (battery removed / dead)
  if (rtc.lostPower()) {
    Serial.println("[RTC] Warning: DS3231 lost power (time may be invalid)");
  }

  initialized = true;
  Serial.println("[RTC] Initialized successfully");
  return true;
}

bool RTC::getTime(time_t& t) {
  if (!initialized) {
    Serial.println("[RTC] Error: not initialized");
    return false;
  }

  for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++) {
    if (attempt > 0) delay(10);

    DateTime now = rtc.now();

    // Convert RTClib DateTime -> struct tm -> epoch
    struct tm timeinfo = {};
    timeinfo.tm_year = now.year() - 1900;
    timeinfo.tm_mon  = now.month() - 1;
    timeinfo.tm_mday = now.day();
    timeinfo.tm_hour = now.hour();
    timeinfo.tm_min  = now.minute();
    timeinfo.tm_sec  = now.second();
    timeinfo.tm_isdst = 0;

    t = mktime(&timeinfo);

    if (t > 0) {
      Serial.printf("[RTC] Read success: %04d-%02d-%02d %02d:%02d:%02d (epoch=%lu)\n",
                    now.year(), now.month(), now.day(),
                    now.hour(), now.minute(), now.second(),
                    (unsigned long)t);
      return true;
    }

    Serial.printf("[RTC] Read attempt %d failed\n", attempt + 1);
  }

  Serial.println("[RTC] Read failed after all retries");
  return false;
}

bool RTC::setTime(time_t t) {
  if (!initialized) {
    Serial.println("[RTC] Error: not initialized");
    return false;
  }

  struct tm* timeinfo = gmtime(&t);
  if (timeinfo == nullptr) {
    Serial.println("[RTC] Error: gmtime conversion failed");
    return false;
  }

  DateTime dt(
    timeinfo->tm_year + 1900,
    timeinfo->tm_mon + 1,
    timeinfo->tm_mday,
    timeinfo->tm_hour,
    timeinfo->tm_min,
    timeinfo->tm_sec
  );

  rtc.adjust(dt);

  Serial.printf("[RTC] Set time success: %04d-%02d-%02d %02d:%02d:%02d\n",
                dt.year(), dt.month(), dt.day(),
                dt.hour(), dt.minute(), dt.second());

  return true;
}

bool RTC::getDateTime(int& year, int& month, int& day,
                      int& hour, int& minute, int& second) {
  if (!initialized) {
    Serial.println("[RTC] Error: not initialized");
    return false;
  }

  for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++) {
    if (attempt > 0) delay(10);

    DateTime now = rtc.now();

    year = now.year();
    month = now.month();
    day = now.day();
    hour = now.hour();
    minute = now.minute();
    second = now.second();

    // Basic sanity check
    if (year >= 2000 && year <= 2100 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
      Serial.printf("[RTC] Read success: %04d-%02d-%02d %02d:%02d:%02d\n",
                    year, month, day, hour, minute, second);
      return true;
    }

    Serial.printf("[RTC] Read attempt %d failed\n", attempt + 1);
  }

  Serial.println("[RTC] Read failed after all retries");
  return false;
}
