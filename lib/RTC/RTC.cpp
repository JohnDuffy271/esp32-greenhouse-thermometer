#include "RTC.h"
#include <DS3231.h>

// Global DS3231 instance
static DS3231 rtcModule;

bool RTC::begin() {
  if (initialized) {
    return true;
  }

  // Initialize I2C on pins SDA=21, SCL=22
  Wire.begin(21, 22);
  Serial.println("[RTC] I2C initialized (SDA=21, SCL=22)");

  // Initialize the DS3231 library
  rtcModule.begin();

  // Probe for DS3231 at default address
  Wire.beginTransmission(I2C_ADDR_DS3231);
  uint8_t error = Wire.endTransmission();

  if (error != 0) {
    Serial.printf("[RTC] Error: DS3231 not found at 0x%02X (error=%d)\n", 
                  I2C_ADDR_DS3231, error);
    return false;
  }

  // Check oscillator status (if not running, warn but don't fail)
  if (!rtcModule.readData(0)) {  // Read control register
    Serial.println("[RTC] Warning: Unable to read DS3231 status");
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

  // Retry logic for I2C glitches
  for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++) {
    if (attempt > 0) {
      delay(10);  // Brief delay before retry
    }

    // Read time from DS3231
    if (rtcModule.readData(1)) {  // 1 = get time
      // Use the get method to retrieve the time
      uint8_t year, month, day, hour, minute, second;
      rtcModule.getTime(second, minute, hour, day, month, year);

      // Validate year (northernwidget uses 2-digit year, add 2000)
      int fullYear = year + 2000;
      
      if (fullYear >= 2000 && fullYear <= 2100 && month >= 1 && month <= 12 && 
          day >= 1 && day <= 31 && hour >= 0 && hour <= 23 && 
          minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
        
        // Convert to time_t (Unix epoch)
        struct tm timeinfo = {0};
        timeinfo.tm_year = fullYear - 1900;
        timeinfo.tm_mon = month - 1;
        timeinfo.tm_mday = day;
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = minute;
        timeinfo.tm_sec = second;
        timeinfo.tm_isdst = 0;  // UTC

        t = mktime(&timeinfo);
        
        if (t > 0) {
          Serial.printf("[RTC] Read success: %04d-%02d-%02d %02d:%02d:%02d (epoch=%lu)\n",
                        fullYear, month, day, hour, minute, second, (unsigned long)t);
          return true;
        }
      }
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

  // Convert time_t to broken-down time
  struct tm* timeinfo = gmtime(&t);
  if (timeinfo == nullptr) {
    Serial.println("[RTC] Error: gmtime conversion failed");
    return false;
  }

  uint8_t year = (timeinfo->tm_year + 1900) - 2000;  // northernwidget uses 2-digit year
  uint8_t month = timeinfo->tm_mon + 1;
  uint8_t day = timeinfo->tm_mday;
  uint8_t hour = timeinfo->tm_hour;
  uint8_t minute = timeinfo->tm_min;
  uint8_t second = timeinfo->tm_sec;

  // Set the time in DS3231
  rtcModule.setTime(second, minute, hour, day, month, year);

  Serial.printf("[RTC] Set time success: %04d-%02d-%02d %02d:%02d:%02d\n",
                year + 2000, month, day, hour, minute, second);

  return true;
}

bool RTC::getDateTime(int& year, int& month, int& day,
                      int& hour, int& minute, int& second) {
  if (!initialized) {
    Serial.println("[RTC] Error: not initialized");
    return false;
  }

  // Retry logic for I2C glitches
  for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++) {
    if (attempt > 0) {
      delay(10);  // Brief delay before retry
    }

    // Read time from DS3231
    if (rtcModule.readData(1)) {  // 1 = get time
      uint8_t sec, min, hr, d, mon, yr;
      rtcModule.getTime(sec, min, hr, d, mon, yr);

      int fullYear = yr + 2000;

      // Validate the datetime
      if (fullYear >= 2000 && fullYear <= 2100 && mon >= 1 && mon <= 12 && 
          d >= 1 && d <= 31 && hr >= 0 && hr <= 23 && 
          min >= 0 && min <= 59 && sec >= 0 && sec <= 59) {
        
        year = fullYear;
        month = mon;
        day = d;
        hour = hr;
        minute = min;
        second = sec;

        Serial.printf("[RTC] Read success: %04d-%02d-%02d %02d:%02d:%02d\n",
                      year, month, day, hour, minute, second);
        return true;
      }
    }

    Serial.printf("[RTC] Read attempt %d failed\n", attempt + 1);
  }

  Serial.println("[RTC] Read failed after all retries");
  return false;
}
