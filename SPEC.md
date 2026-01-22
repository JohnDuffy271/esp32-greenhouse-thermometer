# SPEC.md — ESP32 Spec Starter

## Overview
This firmware is a reusable ESP32 starter template with:
- WiFi connection management
- MQTT connection management (publish/subscribe)
- Interrupt-driven digital input sensing with debounce
- Clear modular structure suitable for future expansion

## Platform
- PlatformIO
- Framework: Arduino
- Board: esp32dev

## Configuration
### Files
- include/config_common.h = committed non-secret defaults
- include/config_example.h = committed template placeholders
- include/config_secrets.h = local secrets (NOT committed)
- include/config.h = local include wrapper (NOT committed)

### Required config values
WiFi:
- WIFI_SSID
- WIFI_PASSWORD

MQTT:
- MQTT_HOST
- MQTT_PORT
- MQTT_USERNAME (optional)
- MQTT_PASSWORD (optional)

Device:
- DEVICE_NAME
- FW_VERSION

Topics:
- MQTT_TOPIC_BOOT
- MQTT_TOPIC_LOG
- MQTT_TOPIC_EVENTS
- MQTT_TOPIC_LWT

## Logging
- Serial logging enabled at 115200 baud
- Logs should include a short prefix per module, e.g.
  - `[WiFi] ...`
  - `[MQTT] ...`
  - `[INT] ...`

## WiFi behaviour
- Attempt connection on boot
- Retry until connected
- Non-blocking where possible (no long delays)
- Expose one-shot flag: `wifiJustConnected()`

## MQTT behaviour
- Connect once WiFi is connected
- Set LWT topic to "offline"
- Publish "online" on connect
- Expose one-shot flag: `mqttJustConnected()`
- Publish boot message once per MQTT connection event

## Interrupt + debounce behaviour
- Support N interrupt-driven inputs
- Inputs are assumed **active-low** by default (configurable later)
- ISR must only set a flag
- Debounce time default: 60ms
- After debounce confirms a stable change, publish an event message


## Main loop responsibilities
- service connection manager
- service interrupts/debounce
- publish events/logs
- avoid heavy logic in main.cpp

## MQTT Contract (Default)

This project uses a simple JSON-based MQTT contract suitable for ESP32 devices.

### Base Topic
All topics are rooted under:
```
MQTT_BASE_TOPIC = "test/esp32"
```

### Publish Topics
- **Boot:** `test/esp32/boot` — published once after MQTT connects
- **Log:** `test/esp32/log` — free-form debug messages
- **Events:** `test/esp32/events` — GPIO interrupt events
- **Response:** `test/esp32/resp` — command acknowledgements / replies
- **Status / LWT:** `test/esp32/status` — heartbeat and Last Will & Testament

### Subscribe Topics
- **Commands:** `test/esp32/cmd` — inbound commands

### Payload Format
All payloads are UTF-8 JSON strings unless otherwise noted.

#### Boot Message (`test/esp32/boot`)
Published once after MQTT connects.

```json
{
  "device": "esp32-spec-starter",
  "version": "0.1.0",
  "status": "online"
}
```

#### Log Message (`test/esp32/log`)
Free-form logging for debugging.

```json
{
  "device": "esp32-spec-starter",
  "msg": "WiFi connected",
  "level": "info"
}
```

#### Event Message (`test/esp32/events`)
Used for GPIO interrupt-driven events.

```json
{
  "device": "esp32-spec-starter",
  "type": "gpio",
  "pin": 27,
  "state": 0,
  "ms": 12345
}
```

#### Response Message (`test/esp32/resp`)
Used for command acknowledgements / replies.

```json
{
  "device": "esp32-spec-starter",
  "resp": "pong"
}
```

#### Status Message / LWT (`test/esp32/status`)
Used for heartbeat and Last Will & Testament.

Online example:
```json
{
  "device": "esp32-spec-starter",
  "version": "0.1.0",
  "uptime_s": 3600,
  "wifi_rssi": -45,
  "mqtt": 1
}
```

Offline example (LWT):
```json
{
  "device": "esp32-spec-starter",
  "status": "offline"
}
```

### Command Payloads

Commands are sent to `test/esp32/cmd` as plain text for now.

#### Supported Commands
- `ping` — device responds with `pong` on `test/esp32/resp`

#### Supported Commands

- `ping` — device responds with `pong` on `test/esp32/resp`

#### Planned Commands

- `led=on` — turn LED on
- `led=off` — turn LED off
- `led=toggle` — toggle LED state

#### Example Command → Response Flow

**Publish to `test/esp32/cmd`:**
```
ping
```

**Device publishes to `test/esp32/resp`:**
```json
{
  "device": "esp32-spec-starter",
  "resp": "pong"
}
```
