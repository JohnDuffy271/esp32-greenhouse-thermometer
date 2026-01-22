#pragma once

#include <Arduino.h>

/**
 * @class SensorDHT22
 * @brief DHT22 temperature and humidity sensor abstraction.
 * 
 * Provides simple interface for reading temperature (°C) and humidity (%)
 * from a DHT22 sensor with error handling.
 */
class SensorDHT22 {
public:
  /**
   * @brief Initialize DHT22 sensor on the specified GPIO pin.
   * 
   * @param pin GPIO pin number where DHT22 is connected.
   */
  void begin(uint8_t pin);

  /**
   * @brief Read temperature and humidity from the sensor (blocking).
   * 
   * Performs a complete read cycle. May block for a few hundred milliseconds
   * if sensor is not ready or if read fails.
   * 
   * @param tempC Output parameter for temperature in degrees Celsius.
   * @param humPct Output parameter for humidity in percent (0-100).
   * @return true if read was successful, false if sensor error or timeout.
   */
  bool read(float& tempC, float& humPct);

  /**
   * @brief Check if sensor is ready to be read (non-blocking).
   * 
   * @return true if sensor is initialized and ready, false otherwise.
   */
  bool isReady() const;

private:
  uint8_t dhtPin = 0xFF;  // Invalid pin by default
  bool initialized = false;
  unsigned long lastReadMs = 0;
  const unsigned long MIN_READ_INTERVAL_MS = 2000;  // DHT22 min 2 sec between reads
};
