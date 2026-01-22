#include "Interrupts.h"
#include <Comms.h>
#include "../../include/config.h"

// Singleton instance
Interrupts* Interrupts::instance = nullptr;

// Static ISR wrapper functions (for up to 8 pins)
void IRAM_ATTR isr0() { if (Interrupts::getInstance()) Interrupts::getInstance()->handlePinChange(0); }
void IRAM_ATTR isr1() { if (Interrupts::getInstance()) Interrupts::getInstance()->handlePinChange(1); }
void IRAM_ATTR isr2() { if (Interrupts::getInstance()) Interrupts::getInstance()->handlePinChange(2); }
void IRAM_ATTR isr3() { if (Interrupts::getInstance()) Interrupts::getInstance()->handlePinChange(3); }
void IRAM_ATTR isr4() { if (Interrupts::getInstance()) Interrupts::getInstance()->handlePinChange(4); }
void IRAM_ATTR isr5() { if (Interrupts::getInstance()) Interrupts::getInstance()->handlePinChange(5); }
void IRAM_ATTR isr6() { if (Interrupts::getInstance()) Interrupts::getInstance()->handlePinChange(6); }
void IRAM_ATTR isr7() { if (Interrupts::getInstance()) Interrupts::getInstance()->handlePinChange(7); }

// Array of ISR function pointers
static void (*isrFunctions[8])() = {
  isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
};

// ============================
// Public: getInstance()
// ============================
Interrupts* Interrupts::getInstance() {
  return instance;
}

// ============================
// Public: begin()
// ============================
void Interrupts::begin(Comms& comms) {
  commsPtr = &comms;
  numPins = INPUT_PIN_COUNT;
  instance = this;  // Set singleton instance
  
  // Initialize debounce states
  for (uint8_t i = 0; i < numPins; i++) {
    debounceStates[i].pin = INPUT_PINS[i];
    debounceStates[i].lastStableState = getLogicalState(INPUT_PINS[i], digitalRead(INPUT_PINS[i]));
    debounceStates[i].currentRead = debounceStates[i].lastStableState;
    debounceStates[i].debounceStart = 0;
    debounceStates[i].isDebouncing = false;
    debounceStates[i].baselineCaptured = false;  // First stable state is baseline, no event
  }
  
  setupInterrupts();
  Serial.println("[INT] Interrupts initialized");
}

// ============================
// Public: loop()
// ============================
void Interrupts::loop() {
  processDebounce();
}

// ============================
// Public: handlePinChange()
// ============================
void Interrupts::handlePinChange(uint8_t channelIdx) {
  // ISR: only set flag, no logging, no heap operations
  if (channelIdx < 8) {
    isrFlags[channelIdx] = true;
  }
}

// ============================
// Private: setupInterrupts()
// ============================
void Interrupts::setupInterrupts() {
  Serial.printf("[INT] Setting up %d interrupt pins\n", numPins);
  
  for (uint8_t i = 0; i < numPins && i < 8; i++) {
    uint8_t pin = INPUT_PINS[i];
    
    // Configure as input with pull-up (for active-low logic)
#if INPUT_ACTIVE_LOW == 1
    pinMode(pin, INPUT_PULLUP);
    // Attach interrupt on falling edge (low pulse)
    attachInterrupt(digitalPinToInterrupt(pin), isrFunctions[i], FALLING);
    Serial.printf("[INT] Pin %d configured as active-low with pull-up\n", pin);
#else
    pinMode(pin, INPUT);
    // Attach interrupt on rising edge (high pulse)
    attachInterrupt(digitalPinToInterrupt(pin), isrFunctions[i], RISING);
    Serial.printf("[INT] Pin %d configured as active-high\n", pin);
#endif
  }
}

// ============================
// Private: processDebounce()
// ============================
void Interrupts::processDebounce() {
  for (uint8_t i = 0; i < numPins; i++) {
    uint8_t pin = debounceStates[i].pin;
    
    // Check if ISR has flagged a change
    if (isrFlags[i]) {
      isrFlags[i] = false;  // Clear the flag
      
      // Start debounce if not already debouncing
      if (!debounceStates[i].isDebouncing) {
        debounceStates[i].isDebouncing = true;
        debounceStates[i].debounceStart = millis();
      }
    }
    
    // While debouncing, read pin state
    if (debounceStates[i].isDebouncing) {
      int rawState = digitalRead(pin);
      bool logicalState = getLogicalState(pin, rawState);
      debounceStates[i].currentRead = logicalState;
      
      // Check if debounce period has elapsed
      unsigned long now = millis();
      unsigned long elapsed = now - debounceStates[i].debounceStart;
      
      if (elapsed >= DEBOUNCE_MS) {
        // Debounce complete, check if state actually changed
        if (logicalState != debounceStates[i].lastStableState) {
          // Stable change detected
          debounceStates[i].lastStableState = logicalState;
          
          // Only publish event if baseline has already been captured
          if (debounceStates[i].baselineCaptured) {
            publishPinEvent(pin, logicalState);
          } else {
            // First stable state is baseline, just capture it
            Serial.printf("[INT] Baseline captured for pin %d (state=%d)\n", pin, (logicalState ? 1 : 0));
            debounceStates[i].baselineCaptured = true;
          }
        }
        
        debounceStates[i].isDebouncing = false;
      }
    }
  }
}

// ============================
// Private: getLogicalState()
// ============================
bool Interrupts::getLogicalState(uint8_t pin, int rawState) {
  // Apply active-low/active-high transformation
  bool logical = (rawState == LOW);  // true when pin is LOW
  
#if INPUT_ACTIVE_LOW == 1
  // Active-low: LOW means "active" (1), HIGH means "inactive" (0)
  return logical;  // true when pin is LOW
#else
  // Active-high: HIGH means "active" (1), LOW means "inactive" (0)
  return !logical;  // true when pin is HIGH
#endif
}

// ============================
// Private: publishPinEvent()
// ============================
void Interrupts::publishPinEvent(uint8_t pin, bool logicalState) {
  if (commsPtr == nullptr) {
    Serial.println("[INT] ERROR: Comms not initialized");
    return;
  }
  
  // Construct JSON event
  String eventJson = "{\"pin\":";
  eventJson += pin;
  eventJson += ",\"state\":";
  eventJson += (logicalState ? 1 : 0);
  eventJson += ",\"ms\":";
  eventJson += millis();
  eventJson += "}";
  
  // Publish via Comms
  bool result = commsPtr->publishEventJson(eventJson.c_str());
  if (result) {
    Serial.printf("[INT] Published event for pin %d (state=%d)\n", pin, (logicalState ? 1 : 0));
  }
}
