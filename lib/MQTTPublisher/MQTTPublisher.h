#pragma once

#include <Arduino.h>
#include <Comms.h>
#include <MinMaxTracker.h>

/**
 * @class MQTTPublisher
 * @brief Publishes greenhouse sensor data as JSON to MQTT topics.
 * 
 * Encapsulates JSON formatting and MQTT publishing for:
 * - Status updates (temperature, humidity, timestamp)
 * - Daily min/max statistics
 * - Temperature threshold alarms
 * 
 * Reuses ConnectionManager via Comms for MQTT operations.
 * RTC is optional; if unavailable, timestamps default to 0.
 */
class MQTTPublisher {
public:
  /**
   * @brief Initialize MQTTPublisher with a reference to Comms.
   * 
   * @param comms Reference to the Comms instance (must outlive MQTTPublisher).
   */
  void begin(Comms& comms);

  /**
   * @brief Publish a status message containing current temp/humidity and timestamp.
   * 
   * Publishes to MQTT_GH_TOPIC_STATUS with JSON payload:
   * {
   *   "device": "esp32-greenhouse-thermometer",
   *   "fw": "0.1.0",
   *   "ts": 1737542445,
   *   "temp_c": 21.5,
   *   "hum_pct": 42.3,
   *   "wake_count": 4
   * }
   * 
   * @param device Device name string (e.g., "esp32-greenhouse-thermometer").
   * @param fw Firmware version string (e.g., "0.1.0").
   * @param ts Unix epoch timestamp in seconds (0 if RTC unavailable).
   * @param tempC Temperature in degrees Celsius.
   * @param humPct Relative humidity in percent (0-100).
   * @param wakeCount Number of wake cycles since boot.
   * @return true if publish succeeded, false otherwise.
   */
  bool publishStatus(const char* device,
                     const char* fw,
                     time_t ts,
                     float tempC,
                     float humPct,
                     uint64_t wakeCount);

  /**
   * @brief Publish daily min/max statistics.
   * 
   * Publishes to MQTT_GH_TOPIC_MINMAX with JSON payload:
   * {
   *   "device": "esp32-greenhouse-thermometer",
   *   "ts": 1737542445,
   *   "reset_yyyymmdd": 20260123,
   *   "min_c": 12.3,
   *   "max_c": 28.7
   * }
   * 
   * @param device Device name string.
   * @param ts Unix epoch timestamp (0 if RTC unavailable).
   * @param stats MinMaxTracker::DailyStats containing min, max, and reset date.
   * @return true if publish succeeded, false otherwise.
   */
  bool publishMinMax(const char* device,
                     time_t ts,
                     const MinMaxTracker::DailyStats& stats);

  /**
   * @brief Publish a temperature threshold alarm.
   * 
   * Publishes to MQTT_GH_TOPIC_ALARM with JSON payload:
   * {
   *   "device": "esp32-greenhouse-thermometer",
   *   "ts": 1737542445,
   *   "type": "LOW",
   *   "temp_c": 0.5,
   *   "threshold_c": 1.0
   * }
   * 
   * @param device Device name string.
   * @param ts Unix epoch timestamp (0 if RTC unavailable).
   * @param type Alarm type string ("LOW" or "HIGH").
   * @param tempC Current temperature in degrees Celsius.
   * @param thresholdC Threshold temperature that was breached.
   * @return true if publish succeeded, false otherwise.
   */
  bool publishAlarm(const char* device,
                    time_t ts,
                    const char* type,
                    float tempC,
                    float thresholdC);

private:
  Comms* comms_ = nullptr;
};
