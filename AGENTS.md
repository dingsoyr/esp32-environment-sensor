# AGENTS.md

## Project overview

This repository contains firmware for a battery-powered ESP32 environmental sensor node.

Current hardware:
- ESP32 Dev Module
- BME280 environmental sensor over I2C
- SDA: GPIO 21
- SCL: GPIO 22
- BME280 may use address 0x76 or 0x77

Current firmware responsibilities:
- Generate a stable device ID from the ESP32 eFuse MAC
- Load persistent device configuration from ESP32 Preferences/NVS
- Read temperature, humidity, and pressure from BME280
- Assign a persistent sequence number to every measurement
- Store measurements in a bounded persistent NVS ring buffer
- Connect briefly to Wi-Fi
- Log Wi-Fi RSSI and connection time
- Track total awake time
- Enter deep sleep between measurement cycles

The sensor initiates communication. Future Raspberry Pi communication will be request/response based, with the Pi returning acknowledgements, configuration changes, and possibly commands in the HTTP response.

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

## Measurement buffering

The firmware currently uses a bounded ring buffer with capacity 16.

Requirements for future changes:
- Preserve sequence numbers.
- Preserve oldest-to-newest ordering.
- Never silently exceed the configured capacity.
- When full, the current policy is to discard the oldest measurement.
- Future ACK handling should only remove measurements confirmed as stored by the Raspberry Pi.
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