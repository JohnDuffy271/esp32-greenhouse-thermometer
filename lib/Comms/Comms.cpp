#include "Comms.h"
#include <ConnectionManager.h>
#include "../../include/config.h"

// ============================
// Public: begin()
// ============================
void Comms::begin(ConnectionManager& cm) {
  cmPtr = &cm;
  Serial.println("[Comms] Initialized");
}

// ============================
// Public: publishBoot()
// ============================
bool Comms::publishBoot(const char* msg) {
  return publishToTopic(MQTT_TOPIC_BOOT, msg);
}

// ============================
// Public: publishLog()
// ============================
bool Comms::publishLog(const char* msg) {
  return publishToTopic(MQTT_TOPIC_LOG, msg);
}

// ============================
// Public: publishEventJson()
// ============================
bool Comms::publishEventJson(const char* json) {
  return publishToTopic(MQTT_TOPIC_EVENTS, json);
}

// ============================
// Public: publishResp()
// ============================
bool Comms::publishResp(const char* msg) {
  return publishToTopic(MQTT_TOPIC_RESP, msg);
}

// ============================
// Public: publishStatus()
// ============================
bool Comms::publishStatus(const char* json) {
  return publishToTopic(MQTT_TOPIC_STATUS, json);
}

// ============================
// Private: publishToTopic()
// ============================
bool Comms::publishToTopic(const char* topic, const char* payload) {
  if (cmPtr == nullptr) {
    Serial.println("[Comms] ERROR: Comms not initialized");
    return false;
  }

  bool result = cmPtr->publish(topic, payload);
  if (!result) {
    Serial.printf("[Comms] Publish failed to %s\n", topic);
  }
  return result;
}
