# Copilot repository instructions

This is a PlatformIO Arduino firmware project for a battery-powered ESP32 environmental sensor.

Before editing:
- Inspect the existing code first.
- Preserve currently working behavior unless explicitly asked to change it.
- Prefer the smallest coherent change.
- Explain substantial architectural changes before implementing them.

Always consider:
- deep-sleep compatibility
- battery consumption
- Wi-Fi active time
- flash wear
- persistent NVS state
- recovery after reset or power loss

Hardware:
- ESP32 Dev Module
- BME280 over I2C
- SDA GPIO 21
- SCL GPIO 22
- BME280 address may be 0x76 or 0x77
- ESP32 GPIO uses 3.3 V logic

Persistence:
- Preferences/NVS stores configuration, measurement sequence, and buffered measurements.
- Do not casually rename or reset persistent keys.
- Keep config persistence keys and semantics compatible unless intentionally migrating them.
- Buffered measurement storage has a format/version contract and ACK-based removal semantics.
- Avoid accidental incompatible storage changes, especially to binary-persisted measurement data.
- Explain migration implications before changing persistent storage layout.

Networking:
- The sensor should initiate communication.
- Network failures must not keep the device awake indefinitely.
- Use bounded timeouts.
- Measurements should be stored before network operations.

Secrets:
- Never expose or commit `include/secrets.h`.
- `include/local_config.h` is also a local ignored file for non-secret environment-specific configuration.
- Use the committed example files as templates for those ignored local files.
- Do not hardcode developer-specific or LAN server URLs into tracked source files.
- Use placeholders in tracked examples.

Validation:
- After firmware/source behavior changes, run the relevant ESP32 debug build.
- When touching server API response parsing or protocol logic, also run the native protocol tests.
- Use the current PlatformIO environments and commands for this repository, for example `pio run -e debug` or `$HOME/.platformio/penv/bin/pio run -e debug`, and `pio test -e native` or `$HOME/.platformio/penv/bin/pio test -e native` when `pio` is not on `PATH`.
- Report build results.
- Do not claim native tests cover hardware, Wi-Fi, storage, or full integration behavior.
- Do not claim physical hardware behavior is verified unless the user has tested it.

Code style:
- Keep code understandable and explicit.
- Avoid unnecessary abstraction.
- Avoid unrelated refactors.