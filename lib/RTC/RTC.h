#pragma once

#include <Arduino.h>
#include <time.h>

/**
 * @class RTC
 * @brief DS3231 I2C Real-Time Clock abstraction.
 * 
 * Provides interface for reading/writing time to a DS3231 RTC module
 * via I2C (SDA=21, SCL=22).
 * 
 * Handles I2C communication glitches with retry logic.
 */
class RTC {
public:
  /**
   * @brief Initialize I2C and DS3231 RTC.
   * 
   * Configures I2C on pins SDA=21, SCL=22 and probes for DS3231.
   * 
   * @return true if DS3231 found and initialized, false otherwise.
   */
  bool begin();

  /**
   * @brief Read current time as Unix epoch (time_t).
   * 
   * Retrieves the current time from DS3231 as seconds since Unix epoch.
   * Includes retry logic for I2C glitches.
   * 
   * @param t Output parameter for Unix epoch time.
   * @return true if read successful, false if DS3231 error or not initialized.
   */
  bool getTime(time_t& t);

  /**
   * @brief Set the current time in DS3231.
   * 
   * Sets DS3231 to the specified Unix epoch time.
   * 
   * @param t Unix epoch time to set.
   * @return true if write successful, false if DS3231 error or not initialized.
   */
  bool setTime(time_t t);

  /**
   * @brief Read current date and time as structured values.
   * 
   * Retrieves year, month, day, hour, minute, second from DS3231.
   * 
   * @param year Output: 4-digit year (e.g., 2026)
   * @param month Output: month 1-12
   * @param day Output: day 1-31
   * @param hour Output: hour 0-23 (UTC)
   * @param minute Output: minute 0-59
   * @param second Output: second 0-59
   * @return true if read successful, false otherwise.
   */
  bool getDateTime(int& year, int& month, int& day, 
                   int& hour, int& minute, int& second);

private:
  bool initialized = false;
  const uint8_t I2C_ADDR_DS3231 = 0x68;  // DS3231 I2C address
  const uint8_t MAX_RETRIES = 2;         // Retry once (2 total attempts)
};
