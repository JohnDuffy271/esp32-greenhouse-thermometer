#pragma once

#include <Arduino.h>
#include <time.h>
#include <cmath>

/**
 * @class MinMaxTracker
 * @brief Daily temperature min/max tracker with 10:00 UTC reset.
 * 
 * Maintains rolling min/max temperature values that reset once per day
 * at 10:00 UTC. Uses RTC time_t as the source of truth.
 * 
 * No dependencies on WiFi, MQTT, or delays.
 */
class MinMaxTracker {
public:
  /**
   * @struct DailyStats
   * @brief Container for daily min/max statistics.
   */
  struct DailyStats {
    float min_temp;           // Minimum temperature recorded today (°C)
    float max_temp;           // Maximum temperature recorded today (°C)
    time_t reset_time;        // Unix epoch time of today's reset (10:00 UTC)
    int date_yyyymmdd;        // Date marker (e.g., 20260122) to track daily reset
  };

  /**
   * @brief Update min/max with a new temperature reading.
   * 
   * Automatically detects when the day rolls past 10:00 UTC and resets
   * the min/max counters. If the current date is different from the stored
   * date and we're at or past 10:00 UTC, a reset occurs.
   * 
   * @param temp_c Temperature reading in degrees Celsius
   * @param current_time Current Unix epoch time from RTC (UTC)
   * 
   * Note: If current_time <= 0, the update is ignored (invalid time).
   */
  void update(float temp_c, time_t current_time);

  /**
   * @brief Get the minimum temperature recorded today.
   * 
   * @return Minimum temperature in °C, or NaN if no valid reading yet.
   */
  float getMin() const;

  /**
   * @brief Get the maximum temperature recorded today.
   * 
   * @return Maximum temperature in °C, or NaN if no valid reading yet.
   */
  float getMax() const;

  /**
   * @brief Check if the provided time represents a new calendar day (at 10:00 UTC).
   * 
   * @param current_time Unix epoch time to evaluate
   * @return true if this is a different day (in yyyymmdd) than the last reset
   */
  bool isNewDay(time_t current_time);

  /**
   * @brief Check if a reset has occurred today (at 10:00 UTC or later).
   * 
   * @param current_time Unix epoch time to evaluate
   * @return true if the current date matches the reset date
   */
  bool wasResetToday(time_t current_time);

  /**
   * @brief Get current daily statistics.
   * 
   * @return DailyStats struct with min, max, reset time, and date
   */
  DailyStats getStats() const;

private:
  float min_temp = NAN;
  float max_temp = NAN;
  time_t reset_time = 0;
  int last_reset_date_yyyymmdd = 0;  // Date of last reset (yyyymmdd format)

  // Helper functions
  int getDateYYYYMMDD(time_t t) const;
  int getHourUTC(time_t t) const;
  bool shouldResetAtTime(time_t current_time);
};
