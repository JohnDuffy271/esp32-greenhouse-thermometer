# SPEC.md — ESP32 Greenhouse Thermometer

## Overview
Battery-powered ESP32 greenhouse thermometer with:
- DHT22 sensor for temperature and humidity monitoring
- I2C RTC module (DS3231) for accurate time tracking
- Deep sleep mode (wake every 30 minutes)
- MQTT publish/subscribe for remote monitoring and control
- Configurable temperature thresholds with alarm publishing
- Daily min/max temperature tracking (resets at 10:00 UTC)

## Platform
- PlatformIO
- Framework: Arduino
- Board: esp32dev

## Hardware
- **Sensor:** DHT22 (analog via GPIO)
- **RTC:** DS3231 (I2C: SDA=21, SCL=22)
- **Wakeup:** External RTC alarm or internal timer

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

Thresholds (configurable via MQTT command):
- TEMP_THRESHOLD_LOW (default 1.0°C)
- TEMP_THRESHOLD_HIGH (default 30.0°C)

Sleep:
- SLEEP_INTERVAL_MINUTES (default 30)
- RESET_HOUR_UTC (default 10)

## Logging
- Serial logging enabled at 115200 baud
- Logs should include a short prefix per module, e.g.
  - `[DHT] ...`
  - `[RTC] ...`
  - `[WiFi] ...`
  - `[MQTT] ...`
  - `[Sleep] ...`

## Wake Cycle Behaviour
1. Device wakes from deep sleep every 30 minutes
2. Read DHT22 (temperature, humidity)
3. Read RTC (current time)
4. Check against min/max daily window (reset at 10:00 UTC)
5. Check if temp exceeds thresholds → publish alarm if triggered
6. Publish status JSON to MQTT
7. Sleep for 30 minutes

## WiFi behaviour
- Attempt connection on boot/wake
- Retry until connected (with backoff)
- Non-blocking where possible
- Expose one-shot flag: `wifiJustConnected()`

## MQTT behaviour
- Connect once WiFi is connected
- Set LWT topic to "offline"
- Publish "online" on connect
- Expose one-shot flag: `mqttJustConnected()`
- Subscribe to command topic on connect
- Handle threshold updates from commands

## Main loop responsibilities
- service connection manager
- read sensors when awake
- update min/max tracker
- publish MQTT messages
- initiate deep sleep
- avoid heavy logic; keep execution tight for battery life

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

Commands are sent to `test/esp32/greenhouse/cmd` as JSON.

#### set_thresholds Command
Updates temperature alarm thresholds.

**Publish to `test/esp32/greenhouse/cmd`:**
```json
{
  "cmd": "set_thresholds",
  "low_c": 2.0,
  "high_c": 28.0
}
```

**Response (published to `test/esp32/greenhouse/status`):**
New thresholds take effect on next wake cycle. Status message confirms new values (optional echo response field).

#### get_config Command
Request current configuration.

**Publish to `test/esp32/greenhouse/cmd`:**
```json
{
  "cmd": "get_config"
}
```

**Response (published to separate response topic or embedded in status):**
Returns current thresholds and settings.

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
