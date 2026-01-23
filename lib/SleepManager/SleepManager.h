#pragma once

#include <Arduino.h>

/**
 * @class SleepManager
 * @brief ESP32 deep sleep scheduler with RTC memory wake counter.
 * 
 * Manages deep sleep intervals (default 30 minutes) with wake-up counter
 * stored in RTC memory (survives deep sleep). Logs wake reason and boot info.
 * 
 * The sleep() method enters deep sleep and does NOT return.
 */
class SleepManager {
public:
  /**
   * @brief Initialize deep sleep scheduler.
   * 
   * Configures timer-based wake-up and reads RTC memory wake counter.
   * Logs wake reason (cold boot, timer wake, etc.) and current wake count.
   * 
   * @param interval_minutes Sleep interval in minutes (default 30)
   */
  void begin(uint32_t interval_minutes = 30);

  /**
   * @brief Enter deep sleep.
   * 
   * Configures timer wake-up for the configured interval and enters
   * deep sleep. This function does NOT return; the ESP32 will reset
   * when the timer expires.
   * 
   * The wake counter in RTC memory is incremented before sleeping.
   */
  void sleep();

  /**
   * @brief Get the current wake counter from RTC memory.
   * 
   * The counter increments each time the device wakes from deep sleep.
   * It survives deep sleep cycles but is reset on power cycle.
   * 
   * @return Wake count (0 = cold boot, 1+ = subsequent wakes)
   */
  uint64_t getWakeCount() const;

  /**
   * @brief Get the configured sleep interval.
   * 
   * @return Sleep interval in minutes
   */
  uint32_t getIntervalMinutes() const;

private:
  uint32_t interval_minutes = 30;
  uint64_t wake_count = 0;

  // RTC memory structure (survives deep sleep, lost on power cycle)
  // RTC_SLOW_MEM: 8KB of memory accessible across sleep cycles
  static const uint32_t RTC_WAKE_COUNT_SLOT = 32;  // Use slot 32 (uint64_t)

  // Helper functions
  void logWakeReason();
  uint64_t readWakeCountFromRTC() const;
  void writeWakeCountToRTC(uint64_t count);
};
