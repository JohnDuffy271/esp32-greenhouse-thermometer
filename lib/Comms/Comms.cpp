#include "Comms.h"
#include <ConnectionManager.h>
#include <config_common.h>
#include <config.h>

void Comms::begin(ConnectionManager& cm) {
  _cm = &cm;
  _mqtt = cm.getMqttClient();

  Serial.println("[Comms] Initialized");
}

bool Comms::publishRaw(const char* topic, const char* payload, bool retained) {
  if (_mqtt == nullptr) {
    Serial.println("[Comms] publishRaw failed: mqtt client is null");
    return false;
  }

  if (!_mqtt->connected()) {
    // Not an error, just means WiFi/MQTT not up yet
    return false;
  }

  bool ok = _mqtt->publish(topic, payload, retained);
  return ok;
}

bool Comms::publishBoot(const char* payload) {
  return publishRaw(MQTT_TOPIC_BOOT, payload, false);
}

bool Comms::publishLog(const char* payload) {
  return publishRaw(MQTT_TOPIC_LOG, payload, false);
}

bool Comms::publishEvents(const char* payload) {
  return publishRaw(MQTT_TOPIC_EVENTS, payload, false);
}

bool Comms::publishResp(const char* payload) {
  return publishRaw(MQTT_TOPIC_RESP, payload, false);
}

bool Comms::publishStatus(const char* payload) {
  return publishRaw(MQTT_TOPIC_STATUS, payload, false);
}

// ============================================================================
// Home Assistant
// ============================================================================

bool Comms::publishHAAvailability(const char* payload, bool retained) {
  return publishRaw(HA_AVAILABILITY_TOPIC, payload, retained);
}

bool Comms::publishHAConfig(bool retained) {
  // Temperature config
  const char* tempConfig =
    "{"
    "\"name\":\"Greenhouse Temperature\","
    "\"unique_id\":\"esp32_greenhouse_temperature\","
    "\"state_topic\":\"" HA_STATE_TOPIC "\","
    "\"value_template\":\"{{ value_json.temp_c }}\","
    "\"unit_of_measurement\":\"°C\","
    "\"device_class\":\"temperature\","
    "\"state_class\":\"measurement\","
    "\"availability_topic\":\"" HA_AVAILABILITY_TOPIC "\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\""
    "}";

  // Humidity config
  const char* humConfig =
    "{"
    "\"name\":\"Greenhouse Humidity\","
    "\"unique_id\":\"esp32_greenhouse_humidity\","
    "\"state_topic\":\"" HA_STATE_TOPIC "\","
    "\"value_template\":\"{{ value_json.humidity_pct }}\","
    "\"unit_of_measurement\":\"%\","
    "\"device_class\":\"humidity\","
    "\"state_class\":\"measurement\","
    "\"availability_topic\":\"" HA_AVAILABILITY_TOPIC "\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\""
    "}";

  bool ok1 = publishRaw(HA_TEMP_CONFIG_TOPIC, tempConfig, retained);
  bool ok2 = publishRaw(HA_HUM_CONFIG_TOPIC, humConfig, retained);

  Serial.printf("[Comms] HA config published: temp=%d hum=%d\n", ok1 ? 1 : 0, ok2 ? 1 : 0);
  return ok1 && ok2;
}

bool Comms::publishEventJson(const char* json, bool retained) {
  if (!_mqtt) return false;
  if (!_mqtt->connected()) return false;
  return _mqtt->publish(MQTT_TOPIC_EVENTS, json, retained);
}


bool Comms::publishHAState(float tempC, float humPct, time_t epoch) {
  char payload[160];

  // epoch can be 0 if RTC not available - still fine
  snprintf(payload, sizeof(payload),
           "{"
           "\"device\":\"%s\","
           "\"temp_c\":%.2f,"
           "\"humidity_pct\":%.2f,"
           "\"epoch\":%lu"
           "}",
           HA_DEVICE_ID,
           tempC,
           humPct,
           (unsigned long)epoch);

  return publishRaw(HA_STATE_TOPIC, payload, false);
}
