#pragma once
#include <Arduino.h>
#include <time.h>
#include <config.h>
#include <config_common.h>

class RTC {
public:
  static bool begin();
  static bool getTime(time_t& t);
  static bool setTime(time_t t);
};
