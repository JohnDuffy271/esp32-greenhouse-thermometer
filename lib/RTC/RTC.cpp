#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include "RTC.h"
#include <config_common.h>

#ifndef I2C_SDA_PIN
 #define I2C_SDA_PIN 21
#endif

#ifndef I2C_SCL_PIN
 #define I2C_SCL_PIN 22
#endif

#ifndef I2C_FREQ_HZ
 #define I2C_FREQ_HZ 100000
#endif 

static RTC_DS3231 rtc;
static bool initialized = false;

bool RTC::begin() {
  if (initialized) return true;

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ);
  Serial.println("[RTC] I2C initialized (SDA=21, SCL=22)");

  if (!rtc.begin()) {
    Serial.println("[RTC] Error: DS3231 not found");
    return false;
  }

  initialized = true;
  Serial.println("[RTC] Initialized successfully");
  return true;
}

bool RTC::getTime(time_t& t) {
  if (!initialized) return false;

  DateTime now = rtc.now();

  struct tm tmNow = {};
  tmNow.tm_year = now.year() - 1900;
  return true;
};
