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

## TASK 004: Min/Max Tracker + Daily Reset (10:00 UTC) ✓
**Status:** Complete  
**Files Created:** `lib/MinMaxTracker/MinMaxTracker.h`, `lib/MinMaxTracker/MinMaxTracker.cpp`  
**Purpose:** Track daily min/max temperatures with automatic UTC reset at 10:00.

**Implementation Notes:**
- Pure logic module: no MQTT, WiFi, delays, or external dependencies
- Uses RTC time_t (Unix epoch) as source of truth, interpreted via gmtime() for UTC
- Stores date marker (yyyymmdd) to ensure exactly one reset per calendar day
- Reset triggers when: hour >= 10 UTC AND current date differs from stored reset date
- On reset: min/max initialized to current reading; reset_time and date_yyyymmdd updated
- Handles invalid time_t values (≤0) gracefully by skipping update
- Logging prefix: `[MinMax]`

**Public Interface Implemented:**
```cpp
void update(float temp_c, time_t current_time)         // Update min/max, check reset
float getMin() const                                    // Get daily minimum (°C)
float getMax() const                                    // Get daily maximum (°C)
bool isNewDay(time_t current_time)                      // Check if new calendar day
bool wasResetToday(time_t current_time)                 // Check if reset occurred today
struct DailyStats {
  float min_temp;                 // Minimum temperature today
  float max_temp;                 // Maximum temperature today
  time_t reset_time;              // Unix epoch of today's 10:00 UTC reset
  int date_yyyymmdd;              // Date marker (e.g., 20260122)
};
DailyStats getStats() const                             // Get full daily statistics
```

**Acceptance Criteria:**
- ✓ Compiles for env:esp32dev
- ✓ Correctly identifies 10:00 UTC reset time using gmtime()
- ✓ Tracks min/max correctly across multiple readings
- ✓ Resets exactly once per day when date changes and hour ≥ 10
- ✓ Handles first reading after boot (initializes NaN → current temp)
- ✓ No blocking delays, pure combinational logic
- ✓ Validates time_t input (ignores ≤ 0)

**Reset Behavior:**
- First reading of day: min = max = current_temp, reset_time = current_time
- Subsequent readings: update min/max normally
- On 10:00 UTC boundary (next day): min/max reset to new reading
- Date marker prevents multiple resets on same day

**Testing Notes:**
- Serial output: `[MinMax] Update: temp=22.5°C, min=15.2°C, max=28.7°C`
- On reset: `[MinMax] Reset triggered at 10:00 UTC (date: 20260123)`
- Invalid time: `[MinMax] Warning: invalid time_t, skipping update`
- getStats() returns complete DailyStats for publishing

---

## TASK 005: Deep Sleep Scheduler (30-min intervals) ✓
**Status:** Complete  
**Files Created:** `lib/SleepManager/SleepManager.h`, `lib/SleepManager/SleepManager.cpp`  
**Purpose:** Manage ESP32 deep sleep and scheduled wake intervals.

**Implementation Notes:**
- Uses real ESP32 deep sleep via `esp_deep_sleep_start()` (does not return)
- Timer-based wake-up configured via `esp_sleep_enable_timer_wakeup()`
- Wake counter stored in RTC slow memory (survives deep sleep, lost on power cycle)
- Uses `RTC_DATA_ATTR` annotation for RTC-persistent variable
- Logs wake reason (cold boot vs timer wake) using `esp_sleep_get_wakeup_cause()`
- Increments wake counter before sleeping
- Default 30-minute interval, configurable via `begin()`
- Logging prefix: `[Sleep]`

**Public Interface Implemented:**
```cpp
void begin(uint32_t interval_minutes = 30)  // Initialize, log wake reason, read wake count
void sleep()                                 // Enter deep sleep (does NOT return)
uint64_t getWakeCount() const               // Get wake counter from RTC memory
uint32_t getIntervalMinutes() const         // Get configured interval
```

**Acceptance Criteria:**
- ✓ Compiles for env:esp32dev
- ✓ ESP32 enters deep sleep via `esp_deep_sleep_start()`
- ✓ Wakes every 30 minutes via timer interrupt
- ✓ Wake counter persists across deep sleep cycles
- ✓ RTC timestamp survives sleep-wake cycle (independent RTC module)
- ✓ Wake reason logged at boot (cold boot vs timer wake)

**Sleep Behavior:**
- **Cold boot:** Wake reason = NONE, wake_count starts at 0
- **Timer wake:** Wake reason = TIMER, wake_count increments by 1
- **Sleep entry:** Increments counter, flushes Serial, calls esp_deep_sleep_start()
- **Timer duration:** interval_minutes × 60 seconds (configurable)

**Logging Output:**
```
[Sleep] Initialized: interval=30 min, wake_count=0
[Sleep] Wake reason: NONE (cold boot)
[Sleep] Entering deep sleep for 30 minutes (wake_count=1)
```

**RTC Memory Details:**
- Uses `RTC_DATA_ATTR` for RTC slow memory (survives deep sleep)
- Counter stored as uint64_t (supports >4 billion wakes)
- Reset on power cycle (not on deep sleep wake)

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


## TASK 005: Deep Sleep Scheduler (30-min intervals)
**Status:** Not started  
**Files to Create:** `lib/SleepManager/SleepManager.h`, `lib/SleepManager/SleepManager.cpp`  
**Files to Modify:** `src/main.cpp`  
**Purpose:** Enter ESP32 deep sleep between sensor readings to support battery operation.

### Requirements
- Deep sleep interval default: 30 minutes
- Store a wake counter that survives deep sleep (RTC memory is fine)
- Log wake reason and wake count on boot
- Provide a `sleep()` method that sleeps for the configured interval
- Logging prefix: `[Sleep]`

### Public Interface
```cpp
class SleepManager {
public:
  static void begin(uint32_t intervalMinutes = 30);
  static void logWakeInfo();
  static uint64_t getWakeCount();
  static uint32_t getIntervalMinutes();
  static void sleep(); // enters deep sleep (does not return)
};

Acceptance Criteria

- Compiles for env:esp32dev
- On every boot: logs wake reason + wake count
- Sleeps for ~30 minutes and wakes repeatedly
- Does not block normal WiFi/MQTT logic (work happens then sleep)

