#pragma once

// ============================
// Configuration Example
// ============================
// This file shows the structure for setting up a new ESP32 device.
//
// 1. Copy include/config_common.h and customize DEVICE_NAME, FW_VERSION, MQTT topics, etc.
// 2. Create include/config_secrets.h with your WiFi and MQTT credentials
// 3. Create include/config.h that includes both files
//
// See config_secrets.h and config_common.h for all available options.

// Example WiFi credentials (from config_secrets.h):
// #define WIFI_SSID       "YOUR_WIFI_SSID"
// #define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// Example MQTT authentication (from config_secrets.h):
// #define MQTT_USERNAME   "YOUR_MQTT_USERNAME"
// #define MQTT_PASSWORD   "YOUR_MQTT_PASSWORD"

// Example MQTT host (from config_common.h):
// #define MQTT_HOST       "192.168.0.20"
// #define MQTT_PORT       1883

// Example device identity (from config_common.h):
// #define DEVICE_NAME     "esp32-spec-starter"
// #define FW_VERSION      "0.1.0"

