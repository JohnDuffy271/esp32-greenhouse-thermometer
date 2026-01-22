#include "SensorDHT22.h"
#include <DHT.h>

// DHT instance pointer (will be created in begin())
static DHT* dhtPtr = nullptr;

void SensorDHT22::begin(uint8_t pin) {
  dhtPin = pin;
  
  // Create DHT instance for DHT22 on specified pin
  if (dhtPtr == nullptr) {
    dhtPtr = new DHT(pin, DHT22);
  }
  
  dhtPtr->begin();
  initialized = true;
  Serial.printf("[DHT] Initialized on pin %d\n", pin);
}

bool SensorDHT22::read(float& tempC, float& humPct) {
  if (!initialized || dhtPtr == nullptr) {
    Serial.println("[DHT] Error: not initialized");
    return false;
  }

  // Respect minimum read interval (DHT22 spec: 2 seconds)
  unsigned long nowMs = millis();
  if (nowMs - lastReadMs < MIN_READ_INTERVAL_MS) {
    Serial.printf("[DHT] Skipping read; %lu ms since last read\n", 
                  nowMs - lastReadMs);
    return false;
  }

  // Perform read
  tempC = dhtPtr->readTemperature();
  humPct = dhtPtr->readHumidity();
  lastReadMs = nowMs;

  // Check if read was successful
  if (isnan(tempC) || isnan(humPct)) {
    Serial.println("[DHT] Read failed: NaN values");
    return false;
  }

  Serial.printf("[DHT] Read success: temp=%.1f°C hum=%.1f%%\n", tempC, humPct);
  return true;
}

bool SensorDHT22::isReady() const {
  return initialized && (dhtPin != 0xFF);
}
