# AGENTS.md

## Project overview

This repository contains firmware for a battery-powered ESP32 environmental sensor node.

Current hardware:
- ESP32 Dev Module
- Seeed Studio XIAO ESP32-C6
- BME280 environmental sensor over I2C
- I2C uses Arduino board-default `Wire.begin()` pins
- ESP32 Dev Module: SDA GPIO 21, SCL GPIO 22
- XIAO ESP32-C6: SDA D4, SCL D5
- BME280 may use address 0x76 or 0x77

PlatformIO environments currently include the existing ESP32 Dev Module targets
and a XIAO ESP32-C6 debug target pinned to `pioarduino` platform `55.03.311`.
That XIAO debug target disables deep sleep so native USB serial stays available
during bring-up. Upload and monitor ports are expected to be provided locally,
not committed. Normal battery-oriented operation will use deep sleep later.

Current firmware responsibilities:
- Generate a stable device ID from the ESP32 eFuse MAC
- Load persistent device configuration from ESP32 Preferences/NVS
- Read temperature, humidity, pressure, and temporary per-measurement battery placeholders
- Assign a persistent sequence number to every measurement
- Store measurements in a bounded persistent NVS ring buffer
- Upload buffered measurements over HTTP
- Validate API v1 upload responses
- Apply server-provided time and configuration updates when valid
- Remove only acknowledged buffered measurements
- Connect briefly to Wi-Fi
- Log Wi-Fi RSSI and connection time
- Track total awake time
- Enter deep sleep between measurement cycles

The sensor initiates communication. The current request/response flow already
supports upload acknowledgements, optional `server_time`, and optional remote
configuration updates in the HTTP response.

## Module ownership

- `main.cpp` orchestrates the wake cycle.
- `server_api_client.cpp` owns HTTP upload and application of validated server responses.
- `server_api_protocol.cpp` owns parsing and validation of the API v1 response.
- `config.cpp` owns persisted device configuration.
- `storage.cpp` owns sequence allocation and persisted buffered measurements.
- `wifi_utils.cpp` and `time_utils.cpp` own Wi-Fi and time-specific behavior.

## Working rules

Before making changes:
1. Inspect the current code and understand existing behavior.
2. Preserve working behavior unless the task explicitly requires changing it.
3. Prefer small, focused changes over broad refactoring.
4. Explain significant architectural changes before implementing them.

After firmware changes:
1. Build with PlatformIO.
2. Report build success or failure.
3. Never claim hardware behavior has been verified unless the user actually tested it on the physical ESP32.

## Safety and hardware rules

- Never assume GPIO assignments. Check the existing project first.
- ESP32 GPIO is 3.3 V logic.
- Do not introduce 5 V signals directly into ESP32 GPIO pins.
- Do not change wiring assumptions without explicitly explaining the required physical change.
- Treat battery life and power consumption as first-class design concerns.

## Power design principles

The production device is intended to spend most of its time in deep sleep.

Prefer:
- short wake cycles
- bounded network timeouts
- minimal delays
- minimal Wi-Fi-on time
- storing measurements before network operations
- graceful failure followed by deep sleep

Avoid:
- long blocking delays
- indefinite Wi-Fi retries
- unnecessary CPU-active time
- excessive flash writes

When adding persistent storage, consider flash wear and explain significant write-frequency changes.

## Persistence

ESP32 Preferences/NVS is currently used for:
- device configuration
- measurement sequence
- persistent measurement buffering

Important persistent state must survive:
- deep sleep
- reset
- firmware restart
- complete power loss

Do not reset, rename, or migrate existing NVS keys casually. If changing persistent layout, explain migration implications first.

Additional storage invariants:
- Persisted configuration keys and meanings must remain compatible unless intentionally migrated.
- Buffered measurement storage has a format/version contract.
- `Measurement` is persisted as binary data, including per-measurement battery fields; layout changes require deliberate compatibility handling.
- Buffered measurements are removed only through ACK-based semantics.
- Storage format/version changes may intentionally clear incompatible buffered data and must not happen accidentally.
- Sequence allocation and buffer behavior are data-loss-sensitive.

## Measurement buffering

The firmware currently uses a bounded ring buffer with capacity 100.

Requirements for future changes:
- Preserve sequence numbers.
- Preserve oldest-to-newest ordering.
- Never silently exceed the configured capacity.
- When full, the current policy is to discard the oldest measurement.
- ACK handling must only remove measurements confirmed by `acknowledged_through`.
- Duplicate transmission must be safe; the Raspberry Pi will eventually deduplicate using device ID + sequence.

## Secrets

Never expose, print, commit, or modify real credentials unnecessarily.

`include/secrets.h` contains local Wi-Fi credentials and is intentionally ignored by Git.

Do not:
- commit it
- copy its values into tracked files
- include credentials in documentation or examples

Use placeholder values in examples.

## Code style

Keep the code simple and readable for a learner maintaining the project.

Prefer:
- descriptive names
- small functions
- explicit behavior
- minimal abstraction until repetition or complexity justifies it

Do not introduce frameworks or elaborate class hierarchies without a concrete need.

## Future direction

Likely future features include:
- Raspberry Pi HTTP API
- acknowledgement-based buffer deletion
- remote configuration
- firmware OTA updates
- battery-voltage reporting
- lower-power production ESP32 board such as FireBeetle
- additional sensors such as wind, rain, and camera nodes

Do not implement future features unless explicitly requested.