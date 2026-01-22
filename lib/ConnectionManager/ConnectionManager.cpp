#include "ConnectionManager.h"
#include <Comms.h>

// Singleton instance
ConnectionManager* ConnectionManager::instance = nullptr;

// Static MQTT callback wrapper
void mqttMessageCallback(char* topic, byte* payload, unsigned int length) {
  if (ConnectionManager::getInstance()) {
    ConnectionManager::getInstance()->handleMqttMessage(topic, payload, length);
  }
}

// ============================
// Constructor
// ============================
ConnectionManager::ConnectionManager() 
  : mqttClient(espClient) {
  // Initialize MQTT client with broker details
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  ledState = false;
}

// ============================
// Public: begin()
// ============================
void ConnectionManager::begin() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[CM] ConnectionManager initialized");
  
  instance = this;  // Set singleton instance
  mqttClient.setCallback(mqttMessageCallback);  // Set MQTT message callback
  
  setupWiFi();
}

// ============================
// Public: loop()
// ============================
void ConnectionManager::loop() {
  // Handle WiFi
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastWiFiAttempt >= WIFI_RETRY_INTERVAL) {
      lastWiFiAttempt = now;
      reconnectWiFi();
    }
  }

  // Handle MQTT (only if WiFi is connected)
  if (wifiConnected()) {
    if (!mqttClient.connected()) {
      unsigned long now = millis();
      if (now - lastMqttAttempt >= MQTT_RETRY_INTERVAL) {
        lastMqttAttempt = now;
        reconnectMqtt();
      }
    } else {
      // Service MQTT client (handle incoming messages, keep-alive)
      mqttClient.loop();
    }
  }
}

// ============================
// Public: wifiConnected()
// ============================
bool ConnectionManager::wifiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

// ============================
// Public: mqttConnected()
// ============================
bool ConnectionManager::mqttConnected() {
  return mqttClient.connected();
}

// ============================
// Public: wifiJustConnected()
// ============================
bool ConnectionManager::wifiJustConnected() {
  bool currentlyConnected = wifiConnected();
  bool justConnected = currentlyConnected && !lastWiFiConnected;
  lastWiFiConnected = currentlyConnected;
  return justConnected;
}

// ============================
// Public: mqttJustConnected()
// ============================
bool ConnectionManager::mqttJustConnected() {
  bool currentlyConnected = mqttConnected();
  bool justConnected = currentlyConnected && !lastMqttConnected;
  lastMqttConnected = currentlyConnected;
  return justConnected;
}

// ============================
// Public: publish()
// ============================
bool ConnectionManager::publish(const char* topic, const char* payload) {
  if (!mqttClient.connected()) {
    Serial.println("[CM] Cannot publish: MQTT not connected");
    return false;
  }
  
  bool result = mqttClient.publish(topic, payload);
  if (!result) {
    Serial.printf("[CM] Publish failed to topic: %s\n", topic);
  }
  return result;
}

// ============================
// Private: setupWiFi()
// ============================
void ConnectionManager::setupWiFi() {
  Serial.println("[CM] Setting up WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[CM] Connecting to SSID: %s\n", WIFI_SSID);
}

// ============================
// Private: reconnectWiFi()
// ============================
void ConnectionManager::reconnectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[CM] WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    return;
  }
  
  // Still trying to connect
  int status = WiFi.status();
  switch (status) {
    case WL_CONNECT_FAILED:
      Serial.println("[CM] WiFi: connection failed, retrying...");
      WiFi.reconnect();
      break;
    case WL_DISCONNECTED:
      Serial.println("[CM] WiFi: disconnected, retrying...");
      WiFi.reconnect();
      break;
    default:
      // Still attempting or scanning, just wait
      break;
  }
}

// ============================
// Private: reconnectMqtt()
// ============================
void ConnectionManager::reconnectMqtt() {
  if (mqttClient.connected()) {
    return;
  }

  Serial.print("[CM] Attempting MQTT connection to ");
  Serial.println(MQTT_HOST);

  // Construct client ID
  String clientId = DEVICE_NAME;
  clientId += "-";
  clientId += random(0xffff);

  // Attempt connection with credentials and LWT
  bool connected = false;
  
  // Use LWT to publish offline/online state
  // When device disconnects, broker will publish "offline" to the LWT topic
  const char* lwt_topic = MQTT_TOPIC_STATUS;
  const char* lwt_payload = "offline";
  
  #ifdef MQTT_USERNAME
    if (strlen(MQTT_USERNAME) > 0) {
      connected = mqttClient.connect(
        clientId.c_str(),
        MQTT_USERNAME,
        MQTT_PASSWORD,
        lwt_topic,
        0,  // QoS
        true,  // retain
        lwt_payload
      );
    } else {
      connected = mqttClient.connect(
        clientId.c_str(),
        lwt_topic,
        0,  // QoS
        true,  // retain
        lwt_payload
      );
    }
  #else
    connected = mqttClient.connect(
      clientId.c_str(),
      lwt_topic,
      0,  // QoS
      true,  // retain
      lwt_payload
    );
  #endif

  if (connected) {
    onMqttConnect();
  } else {
    Serial.print("[CM] MQTT connection failed, rc=");
    Serial.println(mqttClient.state());
  }
}

// ============================
// Private: onMqttConnect()
// ============================
void ConnectionManager::onMqttConnect() {
  Serial.println("[CM] MQTT connected!");
  
  // Publish "online" status to indicate successful connection
  // (overrides the LWT "offline" message set at connection time)
  mqttClient.publish(MQTT_TOPIC_STATUS, "online", true);
  
  // Subscribe to command topic
  mqttClient.subscribe(MQTT_TOPIC_CMD);
  Serial.printf("[CM] Subscribed to: %s\n", MQTT_TOPIC_CMD);
  
  // Boot message is now published by Comms module
}

// ============================
// Public: getInstance()
// ============================
ConnectionManager* ConnectionManager::getInstance() {
  return instance;
}

// ============================
// Public: setComms()
// ============================
void ConnectionManager::setComms(Comms* comms) {
  commsPtr = comms;
}

// ============================
// Private: handleMqttMessage()
// ============================
void ConnectionManager::handleMqttMessage(const char* topic, byte* payload, unsigned int length) {
  // Cap length to prevent overflow
  const unsigned int MAX_PAYLOAD = 256;
  unsigned int safeLength = (length > MAX_PAYLOAD) ? MAX_PAYLOAD : length;
  
  // Convert payload to null-terminated string
  char cmdStr[MAX_PAYLOAD + 1];
  memcpy(cmdStr, payload, safeLength);
  cmdStr[safeLength] = '\0';  // Null terminate
  
  // Trim trailing whitespace (\r \n space tab)
  for (int i = safeLength - 1; i >= 0; i--) {
    if (cmdStr[i] == '\r' || cmdStr[i] == '\n' || cmdStr[i] == ' ' || cmdStr[i] == '\t') {
      cmdStr[i] = '\0';
    } else {
      break;
    }
  }
  
  Serial.printf("[CM] Received message on %s: %s\n", topic, cmdStr);
  
  // Handle the command
  handleCommand(cmdStr);
}

// ============================
// Private: handleCommand()
// ============================
void ConnectionManager::handleCommand(const char* cmdStr) {
  if (cmdStr == nullptr) {
    return;
  }
  
  // Command: "ping" -> respond with "pong"
  if (strcmp(cmdStr, "ping") == 0) {
    Serial.println("[CM] Ping command received");
    if (commsPtr) {
      bool ok = commsPtr->publishResp("pong");
      Serial.printf("[CM] publish pong -> %s\n", ok ? "OK" : "FAILED");
      commsPtr->publishLog("ping received");
    }
    return;
  }
  
  // Command: "led=on"
  if (strcmp(cmdStr, "led=on") == 0) {
    digitalWrite(LED_PIN, HIGH);
    ledState = true;
    Serial.println("[CM] LED turned ON");
    return;
  }
  
  // Command: "led=off"
  if (strcmp(cmdStr, "led=off") == 0) {
    digitalWrite(LED_PIN, LOW);
    ledState = false;
    Serial.println("[CM] LED turned OFF");
    return;
  }
  
  // Command: "led=toggle"
  if (strcmp(cmdStr, "led=toggle") == 0) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    Serial.printf("[CM] LED toggled to %s\n", ledState ? "ON" : "OFF");
    return;
  }
  
  // Unknown command - log it
  Serial.printf("[CM] Unknown command: %s\n", cmdStr);
  if (commsPtr) {
    String logMsg = "[CMD] Unknown command: ";
    logMsg += cmdStr;
    commsPtr->publishLog(logMsg.c_str());
  }
}
