#pragma once

// ============================
// MQTT Broker Host & Port
// ============================
#define MQTT_HOST       "192.168.0.20"
#define MQTT_PORT       1883

// ============================
// RTC Pins for I2C
// ============================

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define I2C_FREQ_HZ 100000

// ============================
// DHT22 Sensor
// ============================
#define DHT_PIN 25


// ============================
// Device Identity
// ============================
#define DEVICE_NAME     "esp32-spec-starter"
#define FW_VERSION      "0.1.0"


// ============================
// Home Assistant MQTT Discovery
// ============================
#define HA_DISCOVERY_PREFIX "homeassistant"
#define HA_DEVICE_ID        "esp32_greenhouse"

// State + availability topics (HA reads these)
#define HA_STATE_TOPIC        "homeassistant/sensor/esp32_greenhouse/state"
#define HA_AVAILABILITY_TOPIC "homeassistant/sensor/esp32_greenhouse/availability"

// Discovery config topics (published retained once)
#define HA_TEMP_CONFIG_TOPIC  "homeassistant/sensor/esp32_greenhouse_temperature/config"
#define HA_HUM_CONFIG_TOPIC   "homeassistant/sensor/esp32_greenhouse_humidity/config"


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
// Greenhouse-specific MQTT Topics
// ============================
#define MQTT_GH_BASE_TOPIC   "test/esp32/greenhouse"
#define MQTT_GH_TOPIC_STATUS "test/esp32/greenhouse/status"
#define MQTT_GH_TOPIC_MINMAX "test/esp32/greenhouse/minmax"
#define MQTT_GH_TOPIC_ALARM  "test/esp32/greenhouse/alarm"

// ============================
// GPIO / LED Output
// ============================
#define LED_PIN 2  // GPIO2 (built-in LED on many ESP32 boards)


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
