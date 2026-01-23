#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

#include "RTC.h"
#include <config_common.h>

static RTC_DS3231 rtc;

// Static member definition
bool RTC::initialized = false;

bool RTC::begin() {
  if (initialized) {
    return true;
  }

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ);
  Serial.printf("[RTC] I2C initialized (SDA=%d, SCL=%d, Freq=%lu)\n",
                I2C_SDA_PIN, I2C_SCL_PIN, (unsigned long)I2C_FREQ_HZ);

  if (!rtc.begin()) {
    Serial.println("[RTC] Error: DS3231 not found");
    initialized = false;
    return false;
  }

  initialized = true;
  Serial.println("[RTC] Initialized successfully");
  return true;
}

bool RTC::getTime(time_t& t) {
  if (!initialized) {
    return false;
  }

  DateTime now = rtc.now();
  t = (time_t)now.unixtime();

  Serial.printf("[RTC] now=%04d-%02d-%02d %02d:%02d:%02d epoch=%lu\n",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second(),
                (unsigned long)t);

  return true;
}

bool RTC::setTime(time_t t) {
  if (!initialized) {
    return false;
  }

  DateTime dt((uint32_t)t);
  rtc.adjust(dt);

  Serial.printf("[RTC] Time set to epoch=%lu\n", (unsigned long)t);
  return true;
}

bool RTC::getDateTime(int& year, int& month, int& day,
                      int& hour, int& minute, int& second) {
  if (!initialized) {
    return false;
  }

  DateTime now = rtc.now();

  year = now.year();
  month = now.month();
  day = now.day();
  hour = now.hour();
  minute = now.minute();
  second = now.second();

  Serial.printf("[RTC] Read success: %04d-%02d-%02d %02d:%02d:%02d\n",
                year, month, day, hour, minute, second);

  return true;
}


static int monthFromStr(const char* m) {
  if (!strncmp(m, "Jan", 3)) return 1;
  if (!strncmp(m, "Feb", 3)) return 2;
  if (!strncmp(m, "Mar", 3)) return 3;
  if (!strncmp(m, "Apr", 3)) return 4;
  if (!strncmp(m, "May", 3)) return 5;
  if (!strncmp(m, "Jun", 3)) return 6;
  if (!strncmp(m, "Jul", 3)) return 7;
  if (!strncmp(m, "Aug", 3)) return 8;
  if (!strncmp(m, "Sep", 3)) return 9;
  if (!strncmp(m, "Oct", 3)) return 10;
  if (!strncmp(m, "Nov", 3)) return 11;
  if (!strncmp(m, "Dec", 3)) return 12;
  return 1;
}

bool RTC::syncToCompileTime() {
  if (!initialized) {
    Serial.println("[RTC] syncToCompileTime() called before begin()");
    return false;
  }

  // __DATE__ = "Mmm dd yyyy"
  // __TIME__ = "hh:mm:ss"
  const char* dateStr = __DATE__;
  const char* timeStr = __TIME__;

  char monStr[4] = { dateStr[0], dateStr[1], dateStr[2], 0 };
  int month = monthFromStr(monStr);

  int day = atoi(dateStr + 4);
  int year = atoi(dateStr + 7);

  int hour = atoi(timeStr);
  int minute = atoi(timeStr + 3);
  int second = atoi(timeStr + 6);

  // DS3231 wants year as 0-99 (2000+)
  uint8_t yy = (uint8_t)(year - 2000);

  Serial.printf("[RTC] Syncing to compile time: %04d-%02d-%02d %02d:%02d:%02d\n",
                year, month, day, hour, minute, second);

  // Write registers directly to avoid library signature differences
  auto decToBcd = [](uint8_t v) -> uint8_t { return (v / 10 * 16) + (v % 10); };

  Wire.beginTransmission(0x68);
  Wire.write(0x00);
  Wire.write(decToBcd((uint8_t)second));
  Wire.write(decToBcd((uint8_t)minute));
  Wire.write(decToBcd((uint8_t)hour)); // 24h
  Wire.write(decToBcd(1));             // day-of-week dummy
  Wire.write(decToBcd((uint8_t)day));
  Wire.write(decToBcd((uint8_t)month));
  Wire.write(decToBcd(yy));
  if (Wire.endTransmission() != 0) {
    Serial.println("[RTC] Sync failed: I2C write error");
    return false;
  }

  Serial.println("[RTC] Sync complete");
  return true;
}