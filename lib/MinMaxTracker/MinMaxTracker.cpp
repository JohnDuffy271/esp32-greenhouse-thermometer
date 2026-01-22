#include "MinMaxTracker.h"

void MinMaxTracker::update(float temp_c, time_t current_time) {
  // Ignore invalid timestamps
  if (current_time <= 0) {
    Serial.println("[MinMax] Warning: invalid time_t, skipping update");
    return;
  }

  // Check if we should reset (crossed 10:00 UTC on a new day)
  if (shouldResetAtTime(current_time)) {
    int today = getDateYYYYMMDD(current_time);
    Serial.printf("[MinMax] Reset triggered at 10:00 UTC (date: %d)\n", today);
    
    // Reset with current reading
    min_temp = temp_c;
    max_temp = temp_c;
    reset_time = current_time;
    last_reset_date_yyyymmdd = today;
    
    Serial.printf("[MinMax] Reset: min=%.1f°C, max=%.1f°C\n", min_temp, max_temp);
    return;
  }

  // Normal update: track min/max
  if (isnan(min_temp) || temp_c < min_temp) {
    min_temp = temp_c;
  }
  if (isnan(max_temp) || temp_c > max_temp) {
    max_temp = temp_c;
  }

  Serial.printf("[MinMax] Update: temp=%.1f°C, min=%.1f°C, max=%.1f°C\n",
                temp_c, min_temp, max_temp);
}

float MinMaxTracker::getMin() const {
  return min_temp;
}

float MinMaxTracker::getMax() const {
  return max_temp;
}

bool MinMaxTracker::isNewDay(time_t current_time) {
  if (current_time <= 0) {
    return false;
  }
  int today = getDateYYYYMMDD(current_time);
  return (today != last_reset_date_yyyymmdd);
}

bool MinMaxTracker::wasResetToday(time_t current_time) {
  if (current_time <= 0) {
    return false;
  }
  int today = getDateYYYYMMDD(current_time);
  return (today == last_reset_date_yyyymmdd);
}

MinMaxTracker::DailyStats MinMaxTracker::getStats() const {
  DailyStats stats;
  stats.min_temp = min_temp;
  stats.max_temp = max_temp;
  stats.reset_time = reset_time;
  stats.date_yyyymmdd = last_reset_date_yyyymmdd;
  return stats;
}

// ============================================================================
// Private helper functions
// ============================================================================

int MinMaxTracker::getDateYYYYMMDD(time_t t) const {
  struct tm* timeinfo = gmtime(&t);
  if (timeinfo == nullptr) {
    return 0;
  }

  int year = timeinfo->tm_year + 1900;
  int month = timeinfo->tm_mon + 1;
  int day = timeinfo->tm_mday;

  return year * 10000 + month * 100 + day;
}

int MinMaxTracker::getHourUTC(time_t t) const {
  struct tm* timeinfo = gmtime(&t);
  if (timeinfo == nullptr) {
    return -1;
  }
  return timeinfo->tm_hour;
}

bool MinMaxTracker::shouldResetAtTime(time_t current_time) {
  if (current_time <= 0) {
    return false;
  }

  int current_hour = getHourUTC(current_time);
  if (current_hour < 0) {
    return false;  // Invalid hour
  }

  int today = getDateYYYYMMDD(current_time);
  
  // Reset should occur when:
  // 1. We've reached or passed 10:00 UTC (hour >= 10)
  // 2. AND the stored date is different (new calendar day)
  bool past_reset_hour = (current_hour >= 10);
  bool new_calendar_day = (today != last_reset_date_yyyymmdd);

  return (past_reset_hour && new_calendar_day);
}
