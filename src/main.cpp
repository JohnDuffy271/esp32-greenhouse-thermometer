#include <Arduino.h>
#include <ConnectionManager.h>
#include <Comms.h>
#include <Interrupts.h>
#include <SensorDHT22.h>
#include <config.h>

// Global instances
ConnectionManager cm;
Comms comms;
Interrupts interrupts;
SensorDHT22 dht22;

// Non-blocking tick scheduler
unsigned long lastTickMs = 0;
const unsigned long TICK_INTERVAL = 100;  // ms

// Heartbeat scheduler
unsigned long lastHeartbeatMs = 0;

// DHT22 reading scheduler (every 30 seconds for demo)
unsigned long lastDHTReadMs = 0;
const unsigned long DHT_READ_INTERVAL_MS = 30000;  // 30 seconds

void setup() {
  // Initialize ConnectionManager (starts WiFi and prepares MQTT)
  cm.begin();
  
  // Initialize Comms with reference to ConnectionManager
  comms.begin(cm);
  
  // Set Comms reference in ConnectionManager for command responses
  cm.setComms(&comms);
  
  // Initialize Interrupts with reference to Comms
  interrupts.begin(comms);
  
  // Initialize DHT22 sensor
  dht22.begin(DHT_PIN);
  
  delay(1000);  // Give WiFi/MQTT a moment to start
  lastTickMs = millis();
  lastHeartbeatMs = millis();  // Initialize heartbeat timer
  lastDHTReadMs = millis();    // Initialize DHT read timer
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
    
    // DHT22 reading scheduler (every 30 seconds for demo)
    if (nowMs - lastDHTReadMs >= DHT_READ_INTERVAL_MS) {
      lastDHTReadMs = nowMs;
      
      float tempC = 0.0f;
      float humPct = 0.0f;
      
      if (dht22.read(tempC, humPct)) {
        // Publish reading to MQTT log topic
        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg), 
                 "Temp=%.1fC Hum=%.1f%%", 
                 tempC, humPct);
        comms.publishLog(logMsg);
      } else {
        comms.publishLog("DHT22 read failed");
      }
    }
    
    // Periodic debug output
    static uint32_t lastPrint = 0;
    if (nowMs - lastPrint >= 5000) {
      lastPrint = nowMs;
      Serial.printf("[DBG] mqttConnected=%d\n", cm.mqttConnected() ? 1 : 0);
    }
  }
  
  // Minimal yield to prevent watchdog trigger
  delay(1);
}