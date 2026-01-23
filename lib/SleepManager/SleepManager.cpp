#include "SleepManager.h"

// RTC memory in slow memory bank (survives deep sleep)
// We use a struct to access RTC slow memory
RTC_DATA_ATTR uint64_t rtc_wake_count = 0;

void SleepManager::begin(uint32_t interval_minutes_param) {
  interval_minutes = interval_minutes_param;
  
  // Read wake count from RTC memory
  wake_count = readWakeCountFromRTC();
  
  Serial.printf("[Sleep] Initialized: interval=%d min, wake_count=%llu\n", 
                interval_minutes, wake_count);
  
  // Log wake reason (cold boot vs timer wake)
  logWakeReason();
}

void SleepManager::sleep() {
  // Increment wake counter and store it in RTC memory
  wake_count++;
  writeWakeCountToRTC(wake_count);
  
  Serial.printf("[Sleep] Entering deep sleep for %d minutes (wake_count=%llu)\n",
                interval_minutes, wake_count);
  Serial.flush();  // Ensure all Serial output is sent before sleep
  
  // Configure timer-based wake-up
  uint64_t sleep_us = (uint64_t)interval_minutes * 60 * 1000000ULL;
  esp_sleep_enable_timer_wakeup(sleep_us);
  
  // Enter deep sleep (does not return)
  esp_deep_sleep_start();
}

uint64_t SleepManager::getWakeCount() const {
  return wake_count;
}

uint32_t SleepManager::getIntervalMinutes() const {
  return interval_minutes;
}

// ============================================================================
// Private helper functions
// ============================================================================

void SleepManager::logWakeReason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  const char* reason_str = "UNKNOWN";
  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0:
    reason_str = "EXT0";
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    reason_str = "EXT1";
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    reason_str = "TIMER";
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    reason_str = "TOUCHPAD";
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    reason_str = "ULP";
    break;
  default:
    reason_str = "COLD BOOT / UNKNOWN";
    break;
  }

  
  Serial.printf("[Sleep] Wake reason: %s\n", reason_str);
}

uint64_t SleepManager::readWakeCountFromRTC() const {
  // Read from RTC memory (persists across deep sleep)
  return rtc_wake_count;
}

void SleepManager::writeWakeCountToRTC(uint64_t count) {
  // Write to RTC memory (persists across deep sleep)
  rtc_wake_count = count;
}
