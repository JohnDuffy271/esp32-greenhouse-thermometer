# Configuration Files

This directory contains configuration files for the esp32-spec-starter project.

## Configuration Structure

The project uses a split-config approach to separate non-secret values from secrets:

### config.h (NOT committed - local only)
- **Purpose:** Main config file loaded by all source code
- **Contents:** Includes both `config_common.h` and `config_secrets.h`
- **Git Status:** Gitignored (each developer has their own)

### config_common.h (committed)
- **Purpose:** Shared, non-secret configuration values
- **Contents:** 
  - Device name and firmware version
  - MQTT broker host/port
  - MQTT topics
  - GPIO pin assignments
  - Heartbeat intervals
  - Interrupt settings
- **Git Status:** Committed to repository

### config_secrets.h (NOT committed - local only)
- **Purpose:** Local secrets (credentials)
- **Contents:**
  - WIFI_SSID and WIFI_PASSWORD
  - MQTT_USERNAME and MQTT_PASSWORD
- **Git Status:** Gitignored (each developer has their own)

### config_example.h (committed)
- **Purpose:** Reference/documentation only
- **Contents:** Placeholder showing the structure
- **Git Status:** Committed to repository (read-only)

## Setup for New Developer

1. Check that `config.h` exists in `include/` (it should be gitignored)
2. Check that `config_secrets.h` exists in `include/` (it should be gitignored)
3. If either file is missing, create them based on these templates:

**config_secrets.h:**
```cpp
#pragma once

#define WIFI_SSID       "YOUR_SSID"
#define WIFI_PASSWORD   "YOUR_PASSWORD"
#define MQTT_USERNAME   "YOUR_MQTT_USER"
#define MQTT_PASSWORD   "YOUR_MQTT_PASSWORD"
```

**config.h:**
```cpp
#pragma once

#include "config_common.h"
#include "config_secrets.h"
```

4. Edit `config_secrets.h` with your actual WiFi and MQTT credentials
5. Optionally, copy and modify `config_common.h` if you need custom device settings

## Why Split Config?

- **Security:** Credentials are never committed to git
- **Clarity:** Common settings are visible to all developers (in config_common.h)
- **Flexibility:** Each developer/device can have different credentials
- **Maintainability:** Shared settings are in one place (config_common.h)

