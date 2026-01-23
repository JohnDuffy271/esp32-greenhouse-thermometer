#pragma once

#include <Arduino.h>
#include <time.h>

class RTC {
public:
  static bool begin();
  static bool getTime(time_t& t);
  static bool setTime(time_t t);
  static bool getDateTime(int& year, int& month, int& day,
                          int& hour, int& minute, int& second);

  static bool syncToCompileTime(); // <-- add this                          

private:
  static bool initialized;
};
