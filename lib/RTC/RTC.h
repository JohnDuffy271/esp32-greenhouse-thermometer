#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <time.h>

class RTC {
public:
  static bool begin(uint8_t sdaPin = 21, uint8_t sclPin = 22);
  static bool getTime(time_t &t);

private:
  static bool _initialized;
};
