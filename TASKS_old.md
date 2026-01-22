# TASKS.md — esp32-spec-starter

## Current Phase
Foundation + Core Modules

---

## Task 001 — Config + skeleton (DONE/VERIFY)
**Goal:** Ensure config separation and a clean starter layout.

### Acceptance criteria
- include/config_example.h exists and compiles when config.h is present
- include/config.h is gitignored
- src/main.cpp builds on ESP32

### Status
- [X] Verify

---

## Task 002 — ConnectionManager (WiFi + MQTT)
**Goal:** Implement a reusable connection manager.

### Deliverables
New module:
- lib/ConnectionManager/ConnectionManager.h
- lib/ConnectionManager/ConnectionManager.cpp

### Behaviour
- egin() sets up WiFi + MQTT client
- loop() services connections
- wifiConnected() / mqttConnected() getters
- one-shot flags:
  - wifiJustConnected()
  - mqttJustConnected()

### Acceptance criteria
- Connects to WiFi
- Connects to MQTT
- Publishes boot message once when MQTT connects
- Compiles and runs on ESP32

### Status
- [X] DONE — ConnectionManager implements full WiFi + MQTT lifecycle with non-blocking reconnect logic. Boot message published automatically on MQTT connect. main.cpp orchestrates via begin()/loop() calls.

---

## Task 003 — Comms module (publish helpers)
**Goal:** Centralise MQTT publishing and topic usage.

### Deliverables
New module:
- lib/Comms/Comms.h
- lib/Comms/Comms.cpp

### Behaviour
- publishLog(const char*)
- publishEventJson(const char*)
- Wrap topic strings from config

### Acceptance criteria
- main.cpp no longer publishes directly
- Compiles cleanly

### Status
- [X] DONE — Comms module created with publishBoot(), publishLog(), publishEventJson(). Delegates to ConnectionManager::publish(). main.cpp now publishes boot message via Comms. No direct MQTT calls in main.

---

## Task 004 — Interrupts + debounce
**Goal:** Interrupt-driven input handling with debounce.

### Deliverables
New module:
- lib/Interrupts/Interrupts.h
- lib/Interrupts/Interrupts.cpp

### Behaviour
- ISR-safe flag set per channel
- debounce state machine (default 60ms)
- stable state change triggers event publish

### Acceptance criteria
- No logging inside ISR
- Pin changes publish to MQTT_TOPIC_EVENTS
- Compiles and runs

### Status
- [X] DONE — Interrupts module with ISR wrapper functions (no logging, no heap). Debounce confirms 60ms stable state before publishing JSON event via Comms. main.cpp calls Interrupts::begin() and loop(). Supports active-low/active-high via config.

---

## Task 004.1 — Baseline capture for interrupts (DONE)
**Goal:** Capture initial pin state as baseline without publishing events.

### Behaviour
- First stable debounced state per pin is baseline (no event published)
- Subsequent state changes publish events as before
- ISR behaviour unchanged (flags only)

### Acceptance criteria
- Boot baseline state is not published as event
- All subsequent changes do publish events
- Interrupt handling unchanged
- Compiles cleanly

### Status
- [X] DONE — Added baselineCaptured flag to DebounceState. First stable state sets baseline; subsequent changes publish events. Compiles cleanly.

---

## Task 005 — MQTT subscribe + command handling
**Goal:** Receive and execute MQTT commands.

### Deliverables
- ConnectionManager subscribes to MQTT_TOPIC_CMD after MQTT connect
- Message callback converts payload safely to null-terminated string
- Command handlers: "ping" (pong), "led=on", "led=off", "led=toggle"
- LED_PIN configured in config.h

### Acceptance criteria
- Commands received via MQTT and executed
- "ping" publishes "pong" to MQTT_TOPIC_LOG
- LED commands control GPIO pin
- Unknown commands logged
- No payload overflow (256 byte cap)
- Compiles for esp32dev

### Status
- [X] DONE — ConnectionManager subscribes to MQTT_TOPIC_CMD with static callback. Payload safely capped at 256 bytes. Handles ping/led commands. LED_PIN (GPIO2) initialized. Unknown commands logged via Comms. main.cpp calls cm.setComms() for responses.

---

## Task 005.1 — Trim MQTT command payload (DONE)
**Goal:** Fix command parsing to trim trailing whitespace from MQTT payloads.

### Behaviour
- MQTT payload trimmed of trailing whitespace (\r \n space tab) before command comparison
- "ping" command now recognized correctly despite whitespace
- Publishes "pong" via Comms::publishLog()
- All other commands unchanged

### Acceptance criteria
- Payload trimmed before strcmp() calls
- "ping" (with trailing whitespace) recognized and responds
- No other behaviour changed
- Compiles for esp32dev

### Status
- [X] DONE — Modified handleMqttMessage() to trim trailing whitespace from cmdStr before handleCommand() call. Loop iterates from end backwards, null-terminating at first non-whitespace character. Build successful, no errors introduced.

---

## Task 005.2 — Remove blocking delay from main loop (MQTT responsiveness)
**Goal:** Ensure MQTT subscriptions and callbacks remain responsive by removing long blocking delays.

### Problem
Using delay(100) in the main loop can starve MQTT processing and cause missed incoming messages.

### Changes
- Replace delay(100) with a non-blocking scheduling pattern using millis()
- Allow a tiny yield (delay(1) or yield()) only

### Acceptance criteria
- cm.loop() is called every loop iteration
- Incoming MQTT messages (e.g. ping) are received reliably
- Interrupt debounce continues to function normally
- No behaviour changes beyond improved responsiveness

### Status
- [X] DONE — Replaced blocking delay(100) with non-blocking millis()-based scheduler. Loop now calls cm.loop() every iteration for maximum responsiveness. Added lastTickMs and TICK_INTERVAL (100ms) for future periodic tasks. Minimal delay(1) kept to yield to watchdog. Compiles for esp32dev.

---

## Task 005.3 — Publish pong to dedicated response topic (separate from logs)
**Goal:** Make MQTT command responses predictable by separating them from general log output.

### Problem
Right now pong is being published on the log topic (test/esp32/log), which mixes responses with normal diagnostic messages and makes MQTT.fx harder to use cleanly.

### Changes
- Add a new MQTT topic constant: MQTT_TOPIC_RESP (example: test/esp32/resp)
- Update both config templates (config_example.h and config.h) to include the new topic
- Add a helper in Comms: publishResp(const char* msg)
- Update the ping command handler to publish "pong" via publishResp() instead of publishLog()
- Logging of "ping received" still uses the existing log topic

### Acceptance criteria
- Sending ping to test/esp32/cmd results in pong published to test/esp32/resp
- Log messages still use test/esp32/log topic as before
- Build passes on esp32dev

### Status
- [X] DONE — Added MQTT_TOPIC_RESP to both config.h and config_example.h (default: "test/esp32/resp"). Added publishResp() method to Comms class. Updated ping handler in ConnectionManager to call comms.publishResp("pong") for the response, with publishLog("ping received") for logging. Build successful for esp32dev.

---

## Task 006 — Add MQTT Heartbeat + Last Will (LWT)
**Goal:** Publish a periodic heartbeat/status message so external systems can monitor liveness, and ensure MQTT Last Will correctly marks the device offline if it drops unexpectedly.

### Requirements
- Publish a heartbeat message every HEARTBEAT_INTERVAL_MS (default 60s)
- Heartbeat should publish to a dedicated topic: MQTT_TOPIC_STATUS
- Payload should be JSON and include at least:
  - device (DEVICE_NAME)
  - ersion (FW_VERSION)
  - uptime_s (seconds since boot)
  - wifi_rssi (RSSI in dBm)
  - mqtt (1 if connected, 0 if not)
- Ensure MQTT LWT topic and payload are used so the broker marks device offline on unexpected disconnect.
- Do not change existing interrupt behaviour or command handling.

### Implementation Notes
- Heartbeat must be non-blocking using millis()
- Heartbeat publish should only happen when MQTT is connected
- Keep serial logging minimal and consistent

### Acceptance Criteria
- On boot: device publishes boot message once
- Every 60s: device publishes heartbeat JSON to MQTT_TOPIC_STATUS
- If ESP32 loses power / WiFi: broker shows device offline via LWT
- Build passes for env:esp32dev

### Status
- [X] DONE — Added publishStatus() method to Comms module. Implemented non-blocking heartbeat scheduler in main.cpp using millis() that publishes JSON with device, version, uptime_s, wifi_rssi, and mqtt fields. Configured ConnectionManager to use MQTT LWT with topic MQTT_TOPIC_STATUS, publishing "offline" as LWT and "online" when connected. No changes to interrupt or command handling. Build successful for esp32dev.

---

## Task 007 — Document MQTT Contract (no behaviour change)
**Goal:** Define a clear MQTT topic + payload contract so future work can be driven by specs instead of code browsing.

### Requirements
- Add an "MQTT Contract" section to SPEC.md
- Define publish/subscribe topics
- Define JSON payload examples for boot/log/events/resp/status
- Define command behaviour for ping -> pong

### Acceptance criteria
- Spec is clear enough that a new developer could publish/subscribe correctly
- No code changes required

### Status
- [X] DONE — Added comprehensive MQTT Contract section to SPEC.md with base topic, publish/subscribe topics, detailed JSON payload examples for all message types (boot, log, events, resp, status/LWT), and command behavior examples. No code files modified.
