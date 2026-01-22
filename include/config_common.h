#pragma once

// ============================
// MQTT Broker Host & Port
// ============================
#define MQTT_HOST       "192.168.0.20"
#define MQTT_PORT       1883

// ============================
// Device Identity
// ============================
#define DEVICE_NAME     "esp32-spec-starter"
#define FW_VERSION      "0.1.0"

// ============================
// MQTT Topics
// ============================
#define MQTT_BASE_TOPIC "test/esp32"

#define MQTT_TOPIC_BOOT   "test/esp32/boot"
#define MQTT_TOPIC_LOG    "test/esp32/log"
#define MQTT_TOPIC_EVENTS "test/esp32/events"
#define MQTT_TOPIC_RESP   "test/esp32/resp"
#define MQTT_TOPIC_LWT    "test/esp32/status"
#define MQTT_TOPIC_CMD    "test/esp32/cmd"
#define MQTT_TOPIC_STATUS "test/esp32/status"

// ============================
// GPIO / LED Output
// ============================
#define LED_PIN 2  // GPIO2 (built-in LED on many ESP32 boards)

// ============================
// DHT22 Sensor
// ============================
#define DHT_PIN 27  // GPIO27 (DHT22 data pin)

// ============================
// Interrupt Inputs
// ============================
// Set INPUT_ACTIVE_LOW to 1 for active-low inputs, 0 for active-high
#define INPUT_ACTIVE_LOW 1
#define INPUT_PIN_COUNT  2
static const uint8_t INPUT_PINS[INPUT_PIN_COUNT] = { 0, 27 };

// ============================
// Heartbeat Configuration
// ============================
#define HEARTBEAT_INTERVAL_MS 60000  // 60 seconds
