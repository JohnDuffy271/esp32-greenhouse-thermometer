#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "../../include/config.h"

// Forward declaration
class Comms;

/**
 * @class ConnectionManager
 * @brief Manages WiFi and MQTT connectivity with one-shot connection flags.
 * 
 * Provides non-blocking connection management and one-shot notification flags
 * for WiFi and MQTT connection events.
 */
class ConnectionManager {
public:
  /**
   * @brief Initialize ConnectionManager (does not connect yet).
   */
  ConnectionManager();

  /**
   * @brief Set up WiFi and MQTT clients with credentials from config.h.
   * 
   * Starts WiFi connection and prepares MQTT client. This is non-blocking;
   * actual connection happens in loop().
   */
  void begin();

  /**
   * @brief Service WiFi and MQTT connections.
   * 
   * Call this regularly from main loop. Handles:
   * - WiFi reconnection attempts
   * - MQTT reconnection attempts
   * - MQTT client polling
   */
  void loop();

  /**
   * @brief Check if WiFi is currently connected.
   * 
   * @return true if connected to SSID, false otherwise.
   */
  bool wifiConnected() const;

  /**
   * @brief Check if MQTT is currently connected.
   * 
   * @return true if connected to broker, false otherwise.
   */
  bool mqttConnected();

  /**
   * @brief One-shot flag: WiFi just connected.
   * 
   * Returns true exactly once after WiFi connects. Subsequent calls return false
   * until WiFi disconnects and reconnects.
   * 
   * @return true if WiFi connected on this call for the first time since last disconnect.
   */
  bool wifiJustConnected();

  /**
   * @brief One-shot flag: MQTT just connected.
   * 
   * Returns true exactly once after MQTT connects. Subsequent calls return false
   * until MQTT disconnects and reconnects.
   * 
   * @return true if MQTT connected on this call for the first time since last disconnect.
   */
  bool mqttJustConnected();

  /**
   * @brief Publish a message to a topic.
   * 
   * @param topic MQTT topic (full path).
   * @param payload Message payload.
   * @return true if publish was successful, false otherwise.
   */
  bool publish(const char* topic, const char* payload);

  /**
   * @brief Set Comms reference for publishing command responses.
   * 
   * @param commsPtr Pointer to Comms instance.
   */
  void setComms(Comms* commsPtr);

  /**
   * @brief Get singleton instance for MQTT callbacks.
   */
  static ConnectionManager* getInstance();

  /**
   * @brief Handle incoming MQTT message (called by callback).
   * 
   * @param topic MQTT topic
   * @param payload Message payload bytes
   * @param length Payload length
   */
  void handleMqttMessage(const char* topic, byte* payload, unsigned int length);

private:
  WiFiClient espClient;
  PubSubClient mqttClient;
  Comms* commsPtr = nullptr;

  bool lastWiFiConnected = false;
  bool lastMqttConnected = false;

  unsigned long lastWiFiAttempt = 0;
  unsigned long lastMqttAttempt = 0;
  const unsigned long WIFI_RETRY_INTERVAL = 5000;  // ms
  const unsigned long MQTT_RETRY_INTERVAL = 3000;  // ms

  static ConnectionManager* instance;
  volatile bool ledState = false;  // Track LED state for toggle

  void setupWiFi();
  void reconnectWiFi();
  void reconnectMqtt();
  void onMqttConnect();
  void handleCommand(const char* cmdStr);
};
