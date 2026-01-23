// Date/time written: 2026-01-23
// Written by ChatGPT
// Searchable reference: GH-THERMO-MAIN-SLEEP-001
// Purpose: Greenhouse thermometer: wake, connect WiFi/MQTT, read RTC + DHT22, publish, then deep sleep.

#include <Arduino.h>
#include <WiFi.h>

#include <ConnectionManager.h>
#include <Comms.h>
#include <Interrupts.h>
#include <SensorDHT22.h>
#include <SleepManager.h>
#include <RTC.h>
#include <MinMaxTracker.h>
#include <MQTTPublisher.h>

#include <config.h>

// Global instances
ConnectionManager cm;
Comms comms;
Interrupts interrupts;
SensorDHT22 dht22;
SleepManager sleepMgr;
MinMaxTracker minMaxTracker;
MQTTPublisher mqttPublisher;

static bool haConfigSent = false;


// Timing
static const uint32_t CONNECT_TIMEOUT_MS = 15000;   // max time to wait for WiFi+MQTT
static const uint32_t MQTT_FLUSH_MS      = 250;     // give MQTT time to transmit before sleep

// Temperature thresholds (Celsius)
static const float DEFAULT_LOW_C  = 1.0f;
static const float DEFAULT_HIGH_C = 30.0f;

// Helpers
static void publishBootOnce();
static void publishReadingAndStatus();
static void flushMqttBriefly();
static void goToSleepNow();

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("[MAIN] Boot");

  // Start modules
  cm.begin();
  comms.begin(cm);
  cm.setComms(&comms);

  // Initialize MQTTPublisher
  mqttPublisher.begin(comms);

  // Interrupts not really needed for this greenhouse node, but harmless:
  interrupts.begin(comms);

  // DHT22
  dht22.begin(DHT_PIN);

  // RTC
  bool rtcOk = RTC::begin();
  if (!rtcOk) {
  Serial.println("[MAIN] RTC begin failed (continuing without RTC)");
  } else {
  #ifdef ENABLE_RTC_TIME_SYNC
    RTC::syncToCompileTime();
  #endif
}


  time_t now = 0;
  bool ok = RTC::getTime(now);

  Serial.printf("[MAIN] RTC getTime ok=%d now=%lu\n", ok ? 1 : 0, (unsigned long)now);


  // Sleep manager (default 30 mins unless you override)
  sleepMgr.begin(5);

  // Wait for WiFi + MQTT (bounded)
  uint32_t startMs = millis();
  while (millis() - startMs < CONNECT_TIMEOUT_MS) {
    cm.loop();
    interrupts.loop();
    delay(1);

    if (cm.mqttConnected()) {
      break;
    }
  }

  if (!cm.mqttConnected()) {
    Serial.println("[MAIN] MQTT not connected within timeout (sleeping anyway)");
    goToSleepNow();
    return;
  }

  // Publish boot + reading
  publishBootOnce();
  publishReadingAndStatus();

  // Flush and sleep
  flushMqttBriefly();
  goToSleepNow();
}

void loop() {
  // We should never really get here in battery mode.
  cm.loop();
  interrupts.loop();
  delay(10);
}

// ============================================================================
// Helpers
// ============================================================================

static void publishBootOnce() {
  
  String bootMsg = "{\"device\":\"";
  bootMsg += DEVICE_NAME;
  bootMsg += "\",\"version\":\"";
  bootMsg += FW_VERSION;
  bootMsg += "\",\"status\":\"online\"}";
  comms.publishBoot(bootMsg.c_str());

  // Home Assistant: mark online + publish discovery config (retained)
  comms.publishHAAvailability("online", true);

  if (!haConfigSent) {
    comms.publishHAConfig(true);
    haConfigSent = true;
  }
 }

static void publishReadingAndStatus() {
  // Read RTC time (epoch)
  time_t nowEpoch = 0;
  bool rtcOk = RTC::getTime(nowEpoch);

  // Read DHT22
  float tempC = 0.0f;
  float humPct = 0.0f;

  if (!dht22.read(tempC, humPct)) {
    comms.publishLog("[MAIN] DHT22 read failed");
    return;
  }

  // Home Assistant: publish sensor state (retained)
  comms.publishHAState(tempC, humPct, true);

  // Update min/max tracker
  if (rtcOk) {
    minMaxTracker.update(tempC, nowEpoch);
  } else {
    // If RTC unavailable, pass current time_t as 0
    minMaxTracker.update(tempC, 0);
  }

  // Get wake count from SleepManager
  uint64_t wakeCount = sleepMgr.getWakeCount();

  // Publish status JSON
  mqttPublisher.publishStatus(
    DEVICE_NAME,
    FW_VERSION,
    nowEpoch,
    tempC,
    humPct,
    wakeCount
  );

  // Publish daily min/max
  MinMaxTracker::DailyStats stats = minMaxTracker.getStats();
  mqttPublisher.publishMinMax(
    DEVICE_NAME,
    nowEpoch,
    stats
  );

  // Check temperature thresholds and publish alarm if breached
  if (tempC < DEFAULT_LOW_C) {
    mqttPublisher.publishAlarm(
      DEVICE_NAME,
      nowEpoch,
      "LOW",
      tempC,
      DEFAULT_LOW_C
    );
  } else if (tempC > DEFAULT_HIGH_C) {
    mqttPublisher.publishAlarm(
      DEVICE_NAME,
      nowEpoch,
      "HIGH",
      tempC,
      DEFAULT_HIGH_C
    );
  }

  // Also publish a human-readable log line
  char logMsg[96];
  snprintf(logMsg, sizeof(logMsg), "Temp=%.1fC Hum=%.1f%%", tempC, humPct);
  comms.publishLog(logMsg);
}

static void flushMqttBriefly() {
  uint32_t t0 = millis();
  while (millis() - t0 < MQTT_FLUSH_MS) {
    cm.loop();
    delay(1);
  }
}

static void goToSleepNow() {
  Serial.println("[MAIN] Sleeping now...");
  Serial.flush();

  sleepMgr.sleep(); // does not return
}
