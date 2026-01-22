#include <Arduino.h>
#include <ConnectionManager.h>
#include <Comms.h>
#include <Interrupts.h>
#include <config.h>

// Global instances
ConnectionManager cm;
Comms comms;
Interrupts interrupts;

// Non-blocking tick scheduler
unsigned long lastTickMs = 0;
const unsigned long TICK_INTERVAL = 100;  // ms

// Heartbeat scheduler
unsigned long lastHeartbeatMs = 0;

void setup() {
  // Initialize ConnectionManager (starts WiFi and prepares MQTT)
  cm.begin();
  
  // Initialize Comms with reference to ConnectionManager
  comms.begin(cm);
  
  // Set Comms reference in ConnectionManager for command responses
  cm.setComms(&comms);
  
  // Initialize Interrupts with reference to Comms
  interrupts.begin(comms);
  
  delay(1000);  // Give WiFi/MQTT a moment to start
  lastTickMs = millis();
  lastHeartbeatMs = millis();  // Initialize heartbeat timer
}

void loop() {
  // Service WiFi and MQTT connections (every iteration for responsiveness)
  cm.loop();
  
  // When MQTT just connected, publish boot message via Comms
  if (cm.mqttJustConnected()) {
    String bootMsg = "{\"device\":\"";
    bootMsg += DEVICE_NAME;
    bootMsg += "\",\"version\":\"";
    bootMsg += FW_VERSION;
    bootMsg += "\",\"status\":\"online\"}";
    comms.publishBoot(bootMsg.c_str());
  }
  
  // Service interrupt debounce state machine
  interrupts.loop();
  
  // Non-blocking heartbeat publishing
  unsigned long nowMs = millis();
  if (cm.mqttConnected() && (nowMs - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS)) {
    lastHeartbeatMs = nowMs;
    
    // Build heartbeat JSON with device info and uptime
    uint32_t uptimeS = nowMs / 1000;
    int32_t rssi = WiFi.RSSI();  // RSSI in dBm
    int mqtt_status = cm.mqttConnected() ? 1 : 0;
    
    String statusJson = "{\"device\":\"";
    statusJson += DEVICE_NAME;
    statusJson += "\",\"version\":\"";
    statusJson += FW_VERSION;
    statusJson += "\",\"uptime_s\":";
    statusJson += uptimeS;
    statusJson += ",\"wifi_rssi\":";
    statusJson += rssi;
    statusJson += ",\"mqtt\":";
    statusJson += mqtt_status;
    statusJson += "}";
    
    comms.publishStatus(statusJson.c_str());
  }
  
  // Non-blocking ~100ms tick for periodic work
  if (nowMs - lastTickMs >= TICK_INTERVAL) {
    lastTickMs = nowMs;
    // Periodic tasks can be added here if needed
    static uint32_t lastPrint = 0;
if  (nowMs - lastPrint >= 5000) {
    lastPrint = nowMs;
    Serial.printf("[DBG] mqttConnected=%d\n", cm.mqttConnected() ? 1 : 0);
}

  }
  
  // Minimal yield to prevent watchdog trigger
  delay(1);
}