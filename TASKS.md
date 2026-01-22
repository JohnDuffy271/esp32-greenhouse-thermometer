# TASKS.md — ESP32 Greenhouse Thermometer

## Overview
Tasks 001–008 implement the ESP32 Greenhouse Thermometer firmware with modular, battery-optimized design.

Each task focuses on a single component and builds toward full system integration.

---

## TASK 001: Specification + Task Planning ✓
**Status:** Complete  
**Files Modified:** SPEC.md, TASKS.md  
**Purpose:** Define project requirements, MQTT contract, and implementation roadmap.

**Deliverables:**
- Complete SPEC.md with hardware, config, wake cycle, and MQTT contract
- TASKS.md with 8 focused implementation tasks
- Greenhouse-specific MQTT topics and payloads documented

**Notes:**
- No code changes in this task
- Ready to begin hardware abstraction layer implementation

---

## TASK 002: DHT22 Module ✓
**Status:** Complete  
**Files Created:** `lib/SensorDHT22/SensorDHT22.h`, `lib/SensorDHT22/SensorDHT22.cpp`  
**Files Modified:** `platformio.ini`, `include/config_common.h`, `src/main.cpp`  
**Purpose:** Abstract DHT22 sensor reading with error handling and data validation.

**Implementation Notes:**
- Uses Adafruit DHT sensor library (v1.4.6)
- Dynamically creates DHT instance in `begin()` to avoid static constructor issues
- Enforces 2-second minimum interval between reads (DHT22 spec)
- Returns false on NaN values or read failures
- Publishes temperature/humidity to MQTT log topic every 30 seconds (demo)
- Logging prefix: `[DHT]`

**Acceptance Criteria:**
- ✓ Compiles for env:esp32dev
- ✓ DHT22 module created with public interface (begin, read, isReady)
- ✓ Integrated into main.cpp with 30-second read cycle
- ✓ Readings published to MQTT log topic via Comms::publishLog()
- ✓ No blocking delays > 5ms during normal operation

**Testing Notes:**
- Serial output shows `[DHT] Read success: temp=X.XC hum=X.X%` every 30 seconds
- MQTT topic `test/esp32/log` receives formatted strings: `Temp=18.4C Hum=62.1%`
- On sensor error: logs `[DHT] Read failed: NaN values` and publishes "DHT22 read failed"

---

## TASK 003: RTC Module (I2C DS3231) ✓
**Status:** Complete  
**Files Created:** `lib/RTC/RTC.h`, `lib/RTC/RTC.cpp`  
**Files Modified:** `platformio.ini`  
**Purpose:** Abstract DS3231 RTC communication via I2C for accurate timekeeping.

**Implementation Notes:**
- Uses northernwidget/DS3231 library (v1.2.0) for robust I2C communication
- Initializes I2C on pins SDA=21, SCL=22 via `Wire.begin(21, 22)`
- Implements retry logic with one retry (MAX_RETRIES=2) for I2C glitches
- 10ms delay between retries to allow I2C to recover
- Validates datetime range before returning (year 2000-2100, valid month/day/time)
- Converts between Unix epoch (time_t) and structured datetime format
- Logging prefix: `[RTC]`

**Public Interface Implemented:**
```cpp
bool begin()                          // Initialize I2C and probe DS3231
bool getTime(time_t& t)              // Read Unix epoch time with retry
bool setTime(time_t t)               // Set Unix epoch time
bool getDateTime(int& year, int& month, int& day, 
                 int& hour, int& minute, int& second)  // Read structured time
```

**Acceptance Criteria:**
- ✓ Compiles for env:esp32dev
- ✓ Communicates with DS3231 over I2C (SDA=21, SCL=22)
- ✓ Probes DS3231 at address 0x68 and fails gracefully if not found
- ✓ Returns valid timestamps (validated against 2000-2100 range)
- ✓ Tolerates I2C communication glitches (retries once on failure)
- ✓ Converts between time_t (Unix epoch) and structured datetime

**Testing Notes:**
- Serial output shows `[RTC] Initialized successfully` on boot
- `getTime()` returns timestamps like `2026-01-22 15:30:45 (epoch=1737542445)`
- On I2C error: logs `[RTC] Read attempt X failed` then retries
- If DS3231 not found: logs error and returns false from begin()
- `setTime()` accepts Unix epoch and configures DS3231

---

## TASK 004: Min/Max Tracker + Daily Reset (10:00 UTC)
**Status:** Not started  
**Files to Create:** `lib/MinMaxTracker/MinMaxTracker.h`, `lib/MinMaxTracker/MinMaxTracker.cpp`  
**Purpose:** Track daily min/max temperatures and reset at 10:00 UTC.

**Requirements:**
- Maintain min/max temperatures over a rolling 24-hour window
- Reset counters at 10:00 UTC each day
- Update min/max on each new reading
- Store last reset time and current date
- Logging prefix: `[MinMax]`

**Public Interface:**
```cpp
class MinMaxTracker {
  void update(float temp_c, time_t current_time);
  float getMin() const;
  float getMax() const;
  bool isNewDay(time_t current_time);
  bool wasResetToday(time_t current_time);
  struct DailyStats {
    float min_temp;
    float max_temp;
    time_t reset_time;
    int date_yyyymmdd;
  };
  DailyStats getStats() const;
};
```

**Acceptance Criteria:**
- Compiles for env:esp32dev
- Correctly identifies 10:00 UTC reset time
- Tracks min/max correctly across multiple readings
- Resets exactly once per day at 10:00 UTC

---

## TASK 005: Deep Sleep Scheduler (30-min intervals)
**Status:** Not started  
**Files to Create:** `lib/SleepManager/SleepManager.h`, `lib/SleepManager/SleepManager.cpp`  
**Purpose:** Manage ESP32 deep sleep and scheduled wake intervals.

**Requirements:**
- Configure ESP32 deep sleep for N minutes (default 30)
- Track wake count / session
- Wake every 30 minutes for sensor reading
- Minimal wake-time overhead
- Logging prefix: `[Sleep]`

**Public Interface:**
```cpp
class SleepManager {
  void begin(uint32_t interval_minutes = 30);
  void sleep();  // Enter deep sleep; does not return
  uint64_t getWakeCount() const;
  uint32_t getIntervalMinutes() const;
};
```

**Acceptance Criteria:**
- Compiles for env:esp32dev
- ESP32 enters and wakes from deep sleep
- Wake interval is accurate (within ±5% tolerance)
- RTC/timestamp survives sleep-wake cycle

---

## TASK 006: MQTT JSON Publishing (status / minmax / alarm)
**Status:** Not started  
**Files to Create:** `lib/MQTTPublisher/MQTTPublisher.h`, `lib/MQTTPublisher/MQTTPublisher.cpp`  
**Purpose:** Publish sensor readings and alarms as structured JSON to MQTT.

**Requirements:**
- Publish status JSON with temperature, humidity, device info
- Publish minmax JSON with daily min/max statistics
- Publish alarm JSON if temperature exceeds thresholds
- Use base topic: `test/esp32/greenhouse`
- Reuse existing MQTT connection from ConnectionManager
- Logging prefix: `[Pub]`

**Public Interface:**
```cpp
class MQTTPublisher {
  void begin(MQTTConnection* mqtt_conn);
  
  // Publish status (temp, humidity, wifi, timestamp)
  bool publishStatus(float temp_c, float humidity_pct, 
                     int wifi_rssi, time_t timestamp);
  
  // Publish daily min/max
  bool publishMinMax(const MinMaxTracker::DailyStats& stats);
  
  // Publish alarm (only if threshold exceeded)
  bool publishAlarm(float temp_c, const char* alarm_type, 
                    float threshold_c, time_t timestamp);
};
```

**Acceptance Criteria:**
- Compiles for env:esp32dev
- Publishes valid JSON to correct MQTT topics
- Handles MQTT disconnection gracefully
- No payload exceeds 512 bytes

---

## TASK 007: MQTT Command Handler (set_thresholds)
**Status:** Not started  
**Files to Create:** `lib/CommandHandler/CommandHandler.h`, `lib/CommandHandler/CommandHandler.cpp`  
**Purpose:** Parse and execute MQTT commands for threshold configuration.

**Requirements:**
- Subscribe to `test/esp32/greenhouse/cmd`
- Parse inbound JSON commands (set_thresholds, get_config)
- Update threshold values in persistent storage (EEPROM/NVS)
- Handle malformed JSON gracefully
- Logging prefix: `[Cmd]`

**Public Interface:**
```cpp
class CommandHandler {
  void begin(MQTTConnection* mqtt_conn);
  void onMQTTMessage(const char* payload, uint16_t length);
  
  float getLowThreshold() const;
  float getHighThreshold() const;
  void setThresholds(float low_c, float high_c);
};
```

**Command Format:**
```json
{
  "cmd": "set_thresholds",
  "low_c": 2.0,
  "high_c": 28.0
}
```

**Acceptance Criteria:**
- Compiles for env:esp32dev
- Correctly parses valid JSON commands
- Updates thresholds and persists to NVS
- Rejects malformed JSON without crashing
- Default thresholds: LOW=1.0°C, HIGH=30.0°C

---

## TASK 008: Integration + Manual Test Checklist
**Status:** Not started  
**Files to Modify:** `src/main.cpp` (minimal integration code)  
**Purpose:** Integrate all modules into a cohesive working system and validate with manual MQTT testing.

**Requirements:**
- Orchestrate all modules in `setup()` and `loop()`
- Wake cycle: read sensors → update min/max → check alarms → publish MQTT → sleep
- Keep `loop()` tight and non-blocking
- Test all MQTT payloads with MQTT.fx or similar client
- Confirm deep sleep behavior

**Integration Checklist:**
- [ ] DHT22 reads temperature and humidity
- [ ] RTC provides valid timestamps
- [ ] MinMaxTracker correctly updates min/max
- [ ] status JSON publishes on every wake
- [ ] minmax JSON publishes on every wake
- [ ] alarm JSON publishes only when threshold exceeded
- [ ] Command handler receives and parses set_thresholds
- [ ] Thresholds persist across reboot (NVS)
- [ ] Deep sleep wakes every 30 minutes
- [ ] WiFi reconnects if lost
- [ ] MQTT reconnects if lost
- [ ] Power consumption is optimized (minimal wake time)

**Manual Test Scenarios (MQTT.fx):**
1. **Monitor Status:** Subscribe to `test/esp32/greenhouse/#` and confirm messages appear every 30 min
2. **Set Thresholds:** Publish `{"cmd":"set_thresholds","low_c":5.0,"high_c":25.0}` to `test/esp32/greenhouse/cmd`
3. **Trigger Alarm:** Cool/heat sensor past threshold and verify alarm JSON is published
4. **Verify Timestamps:** Check all JSON messages have valid ISO8601 timestamps
5. **Sleep Duration:** Log wake times and verify 30-minute intervals

**Acceptance Criteria:**
- All modules integrated into main loop
- All 8 MQTT payloads validated against SPEC.md
- Manual test checklist passed
- Code compiles and runs for at least 2 complete wake cycles
- Firmware ready for long-term deployment testing

---

## Summary
- **TASK 001:** Specification complete ✓
- **TASK 002–007:** Hardware + comms modules (6 tasks)
- **TASK 008:** System integration + validation

Total estimated effort: ~8–10 developer days (assuming ~1 day per task + integration overhead).

