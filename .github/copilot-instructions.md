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
- Explain migration implications before changing persistent storage layout.

Networking:
- The sensor should initiate communication.
- Network failures must not keep the device awake indefinitely.
- Use bounded timeouts.
- Measurements should be stored before network operations.

Secrets:
- Never expose or commit `include/secrets.h`.
- Use placeholders in tracked examples.

Validation:
- After code changes, run `platformio run`.
- Report build results.
- Do not claim physical hardware behavior is verified unless the user has tested it.

Code style:
- Keep code understandable and explicit.
- Avoid unnecessary abstraction.
- Avoid unrelated refactors.