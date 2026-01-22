#pragma once

#include <Arduino.h>
#include <cstdint>

// Forward declaration
class Comms;

/**
 * @class Interrupts
 * @brief Interrupt-driven input handler with debounce.
 * 
 * Manages edge-triggered interrupt inputs with software debounce.
 * ISRs set flags only (no logging, no heap allocation).
 * Debounce confirms stable state changes before publishing events.
 */
class Interrupts {
public:
  /**
   * @brief Initialize Interrupts module.
   * 
   * @param comms Reference to Comms instance for publishing events.
   */
  void begin(Comms& comms);

  /**
   * @brief Service debounce state machine.
   * 
   * Call regularly from main loop. Processes ISR flags and publishes
   * events when stable state changes are confirmed.
   */
  void loop();

  /**
   * @brief Get singleton instance (for ISR callbacks).
   */
  static Interrupts* getInstance();

  /**
   * @brief ISR callback for pin change.
   * 
   * Called by interrupt handler. Sets flag for corresponding channel.
   * Must be called only from ISR context.
   * 
   * @param channelIdx Index into INPUT_PINS array (0-based).
   */
  void handlePinChange(uint8_t channelIdx);

private:
  Comms* commsPtr = nullptr;

  // ISR flags (one per configured input pin)
  volatile bool isrFlags[8] = {};
  
  // Debounce state per pin
  struct DebounceState {
    uint8_t pin;
    bool lastStableState;
    bool currentRead;
    unsigned long debounceStart;
    bool isDebouncing;
    bool baselineCaptured;  // Track if first stable state has been captured
  };
  
  DebounceState debounceStates[8];  // Support up to 8 pins
  uint8_t numPins = 0;

  // Debounce timing
  const unsigned long DEBOUNCE_MS = 60;

  // Singleton instance
  static Interrupts* instance;

  void setupInterrupts();
  void processDebounce();
  bool getLogicalState(uint8_t pin, int rawState);
  void publishPinEvent(uint8_t pin, bool logicalState);
};

