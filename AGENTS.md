# AGENTS.md — esp32-spec-starter

## Goal
Build an ESP32 PlatformIO (Arduino framework) starter project using a **spec + tasks** workflow.
The code should be modular, readable, and safe to extend.

## Non-negotiables
- Do NOT hardcode secrets in committed files.
- Do NOT break existing behaviour when refactoring.
- Every task must compile for `env:esp32dev`.
- Keep modules small and single-purpose.

## Working rules (Copilot / Chat workflow)
When implementing a task:
1. Read SPEC.md and TASKS.md
2. Implement only what Task N requests
3. Keep changes minimal
4. Build check (`pio run`)
5. Update TASKS.md with completion notes

## Repo structure expectations
- `src/main.cpp` = orchestration only (setup/loop)
- Modules live in `lib/<ModuleName>/`
- Config template in `include/config_example.h`
- Real config in `include/config.h` (gitignored)

## Modules planned
- DeviceInfo (name/version helpers)
- ConnectionManager (WiFi + MQTT)
- Interrupts (ISR flags + debounce)
- Comms (MQTT publish/subscribe helpers)

## Quality checklist
- No blocking delays in loop (except tiny debounce timing)
- ISR functions: set flags only, no logging, no heap
- MQTT reconnect is resilient
- Serial logging is consistent and readable
