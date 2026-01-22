#pragma once

#include <Arduino.h>

// Forward declaration
class ConnectionManager;

/**
 * @class Comms
 * @brief MQTT communication wrapper with topic management.
 * 
 * Provides high-level publish methods that abstract away topic construction
 * and delegate to ConnectionManager's MQTT client.
 */
class Comms {
public:
  /**
   * @brief Initialize Comms with a reference to ConnectionManager.
   * 
   * @param cm Reference to the ConnectionManager instance.
   */
  void begin(ConnectionManager& cm);

  /**
   * @brief Publish a boot message.
   * 
   * @param msg Boot message payload.
   * @return true if successful, false otherwise.
   */
  bool publishBoot(const char* msg);

  /**
   * @brief Publish a log message.
   * 
   * @param msg Log message payload.
   * @return true if successful, false otherwise.
   */
  bool publishLog(const char* msg);

  /**
   * @brief Publish an event as JSON.
   * 
   * @param json JSON event payload.
   * @return true if successful, false otherwise.
   */
  bool publishEventJson(const char* json);

  /**
   * @brief Publish a response message (command replies).
   * 
   * @param msg Response message payload.
   * @return true if successful, false otherwise.
   */
  bool publishResp(const char* msg);

  /**
   * @brief Publish a status/heartbeat message as JSON.
   * 
   * @param json Status JSON payload.
   * @return true if successful, false otherwise.
   */
  bool publishStatus(const char* json);

private:
  ConnectionManager* cmPtr = nullptr;

  bool publishToTopic(const char* topic, const char* payload);
};
