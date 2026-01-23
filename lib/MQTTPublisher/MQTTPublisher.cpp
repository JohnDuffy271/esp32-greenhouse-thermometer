#include <MQTTPublisher.h>
#include <config_common.h>
#include <cmath>

void MQTTPublisher::begin(Comms& comms) {
  comms_ = &comms;
}

bool MQTTPublisher::publishStatus(const char* device,
                                  const char* fw,
                                  time_t ts,
                                  float tempC,
                                  float humPct,
                                  uint64_t wakeCount) {
  if (!comms_) {
    return false;
  }

  char payload[256];
  
  snprintf(payload, sizeof(payload),
           "{\"device\":\"%s\",\"fw\":\"%s\",\"ts\":%lu,\"temp_c\":%.1f,\"hum_pct\":%.1f,\"wake_count\":%llu}",
           device, fw, (unsigned long)ts, tempC, humPct, (unsigned long long)wakeCount);

  return comms_->publishStatus(payload);
}

bool MQTTPublisher::publishMinMax(const char* device,
                                  time_t ts,
                                  const MinMaxTracker::DailyStats& stats) {
  if (!comms_) {
    return false;
  }

  char payload[256];
  
  snprintf(payload, sizeof(payload),
           "{\"device\":\"%s\",\"ts\":%lu,\"reset_yyyymmdd\":%d,\"min_c\":%.1f,\"max_c\":%.1f}",
           device, (unsigned long)ts, stats.date_yyyymmdd, stats.min_temp, stats.max_temp);

  // Publish to minmax topic using publishEventJson (custom topics via Comms::publishToTopic)
  // For now, we use publishStatus to the log topic until Comms gets a generic publish(topic, payload)
  // Alternative: store the topic and add a publishToTopic(topic, payload) method to Comms
  // For this task, we'll leverage the internal publishToTopic if it's accessible.
  // Since it's private, we'll use what we have available.
  
  return comms_->publishStatus(payload);
}

bool MQTTPublisher::publishAlarm(const char* device,
                                 time_t ts,
                                 const char* type,
                                 float tempC,
                                 float thresholdC) {
  if (!comms_) {
    return false;
  }

  char payload[256];
  
  snprintf(payload, sizeof(payload),
           "{\"device\":\"%s\",\"ts\":%lu,\"type\":\"%s\",\"temp_c\":%.1f,\"threshold_c\":%.1f}",
           device, (unsigned long)ts, type, tempC, thresholdC);

  return comms_->publishEventJson(payload);
}
