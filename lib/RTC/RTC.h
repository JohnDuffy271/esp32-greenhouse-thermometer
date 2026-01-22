#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include <stdint.h>

class RTC {
public:
  static bool begin();
  static bool getTime(time_t& t);
  static bool setTime(time_t t);
  static bool getDateTime(int& year, int& month, int& day,
                          int& hour, int& minute, int& second);

private:
  static bool initialized;
  static const uint8_t MAX_RETRIES = 2;
};
