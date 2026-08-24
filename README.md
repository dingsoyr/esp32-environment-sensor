# Environment Sensor Firmware

Firmware for the ESP32-based environment sensor.

Supported boards:

- ESP32 Dev Module
- Seeed Studio XIAO ESP32-C6

## Current firmware runtime

On each wake cycle the firmware:

- loads persisted device configuration
- checks whether the current UTC time is already valid
- connects to Wi-Fi for NTP only when time synchronization is needed
- reads and stores a measurement, including per-measurement battery data, before network upload
- connects to Wi-Fi, or reuses the existing connection from NTP sync
- uploads buffered measurements to the server
- validates the API v1 response
- applies `server_time` when a valid value is returned
- removes only measurements acknowledged by `acknowledged_through`
- persists a newer server-supplied configuration when present
- disconnects Wi-Fi
- enters deep sleep using the effective measurement interval

This README stays focused on setup and day-to-day development. See the source
for implementation details.

## Development environment

Development is done in Ubuntu on WSL2 using Visual Studio Code and
PlatformIO.

The project should be cloned inside the WSL filesystem, for example:

``` bash
~/projects/environment-sensor
```

Avoid developing from `/mnt/c/...`.

## Requirements

### Windows

-   WSL2 with Ubuntu
-   Visual Studio Code
-   `usbipd-win`
-   VS Code WSL extension

`usbipd-win` is used to make the ESP32 USB device available inside WSL.

### WSL / Ubuntu

-   Git
-   Python 3
-   Visual Studio Code PlatformIO IDE extension
-   Visual Studio Code C/C++ extension

## ESP32 USB access from WSL

The ESP32 is connected physically to the Windows machine and passed
through to WSL using `usbipd-win`.

### Initial setup

Connect the ESP32 and open PowerShell as Administrator:

``` powershell
usbipd list
```

Find the BUSID belonging to the ESP32, for example `2-4`.

Bind the device:

``` powershell
usbipd bind --busid 2-4
```

Binding normally only needs to be done once for the device.

Attach the device to WSL:

``` powershell
usbipd attach --wsl --busid 2-4
```

Replace `2-4` with the BUSID shown by `usbipd list`.

### After restarting Windows

The USB device normally needs to be attached to WSL again:

``` powershell
usbipd list
usbipd attach --wsl --busid <BUSID>
```

The BUSID may change, for example if the ESP32 is connected to another
USB port, so check `usbipd list` first.

The device should then be visible inside WSL:

``` bash
lsusb
```

Check for the serial device:

``` bash
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

Depending on the ESP32 board, it may appear as something such as
`/dev/ttyUSB0` or `/dev/ttyACM0`.

PlatformIO can also verify the device:

``` bash
pio device list
```

If `pio` is not on `PATH` in the current shell, use the explicit PlatformIO
virtual environment path instead, for example:

``` bash
$HOME/.platformio/penv/bin/pio device list
```

## Opening the project

From the project directory in WSL:

``` bash
code .
```

Verify that VS Code shows `WSL: Ubuntu` as the remote environment.

PlatformIO IDE and the Microsoft C/C++ extension should be installed in
the WSL environment, not only in the local Windows VS Code environment.

## First-time setup after cloning

Clone the repository into the WSL filesystem and create the two local
configuration files before building or uploading:

``` bash
git clone <repository-url>
cd esp32-environment-sensor

cp include/secrets.example.h include/secrets.h
cp include/local_config.example.h include/local_config.h
```

Both local files must be edited for the current environment before the
firmware is built or uploaded.

## Local configuration files

### Local Wi-Fi secrets

`include/secrets.h` is a local file and is intentionally ignored by Git.
The committed template is `include/secrets.example.h`.

After cloning, create the local file from the example:

``` bash
cp include/secrets.example.h include/secrets.h
```

Edit `include/secrets.h` with the local Wi-Fi SSID and password. Never
commit `include/secrets.h`.

### Local server configuration

`include/local_config.h` contains non-secret, environment-specific local
settings and is intentionally ignored by Git. The committed template is
`include/local_config.example.h`.

After cloning, create the local file from the example:

``` bash
cp include/local_config.example.h include/local_config.h
```

Edit `ENVIRONMENT_SENSOR_SERVER_MEASUREMENTS_URL` so it points to the
current development or deployment server.

Example development value:

``` cpp
constexpr char ENVIRONMENT_SENSOR_SERVER_MEASUREMENTS_URL[] =
    "http://192.168.x.x:8000/api/v1/measurements";
```

The IP above is only a placeholder. A developer-specific LAN IP should
not be committed. When the Raspberry Pi server is deployed, change this
value to the Pi address or hostname. This URL is not a secret, but it is
local environment configuration and should remain in
`include/local_config.h` rather than tracked source files.

## PlatformIO environments

The project currently defines five PlatformIO environments:

-   `debug`: ESP32 Dev Module build with `DEBUG_LOGGING=1` on the official pinned `espressif32@7.0.1` platform.
-   `release`: ESP32 Dev Module build with `DEBUG_LOGGING=0` on the official pinned `espressif32@7.0.1` platform.
-   `xiao_esp32c6_debug`: Seeed Studio XIAO ESP32-C6 build on pinned `pioarduino` platform `55.03.311`, with `DEBUG_LOGGING=1` and `DISABLE_DEEP_SLEEP=1`.
-   `xiao_esp32c6_release`: Seeed Studio XIAO ESP32-C6 build on pinned `pioarduino` platform `55.03.311`, with `DEBUG_LOGGING=0` and normal deep-sleep behavior.
-   `native`: host-side tests of the API response/protocol parser only.

The firmware initializes I2C with `Wire.begin()` so each Arduino board variant
provides its default sensor pins:

-   ESP32 Dev Module: SDA `GPIO21`, SCL `GPIO22`
-   XIAO ESP32-C6: SDA `D4`, SCL `D5`

Buffered measurements are persisted in NVS as raw `Measurement` records. That
record includes `battery_voltage` and `battery_percent`, and those values are
uploaded inside each measurement object. The current firmware still uses
temporary deterministic dummy battery values pending real hardware battery
sensing.

## ESP32 Dev Module

Use the existing ESP32 Dev Module environments for the original board family:

-   `debug`
-   `release`

These environments use the official pinned PlatformIO Espressif32 platform
`espressif32@7.0.1`.

For normal ESP32 Dev Module development, the PlatformIO buttons in VS Code can
be used for Build, Upload, and Serial Monitor as long as the correct
environment is selected.

### VS Code workflow

-   Build using the selected `debug` or `release` environment.
-   Upload using the selected `debug` or `release` environment.
-   Open Serial Monitor for the selected environment when a USB serial port is
        attached to WSL.

### CLI build, upload, and monitor

Debug build:

``` bash
$HOME/.platformio/penv/bin/pio run -e debug
```

Debug upload:

``` bash
$HOME/.platformio/penv/bin/pio run -e debug -t upload
```

Debug serial monitor:

``` bash
$HOME/.platformio/penv/bin/pio device monitor -e debug
```

Release build:

``` bash
$HOME/.platformio/penv/bin/pio run -e release
```

Release upload:

``` bash
$HOME/.platformio/penv/bin/pio run -e release -t upload
```

Release serial monitor:

``` bash
$HOME/.platformio/penv/bin/pio device monitor -e release
```

Supply upload or monitor ports locally when needed, for example with
`--upload-port` or `--port`, rather than committing board-specific device
paths.

## XIAO ESP32-C6

Use the XIAO-specific environments for the Seeed Studio XIAO ESP32-C6:

-   `xiao_esp32c6_debug`
-   `xiao_esp32c6_release`

### Debug and release behavior

`xiao_esp32c6_debug` is the validated bring-up and USB/serial debugging
environment:

-   `DEBUG_LOGGING` enabled
-   deep sleep intentionally disabled
-   intended for bring-up and native USB serial debugging
-   performs one normal firmware cycle and then remains awake

`xiao_esp32c6_release` keeps the normal sensor behavior:

-   `DEBUG_LOGGING` disabled
-   deep sleep enabled
-   intended for normal sensor operation and later battery testing

### Dedicated PlatformIO package cache

The XIAO environments use the pinned `pioarduino` Espressif32 platform, while
the ESP32 Dev Module environments use the official PlatformIO Espressif32
platform. In the current development setup, run the XIAO environments with a
dedicated package cache:

``` bash
PLATFORMIO_PACKAGES_DIR="$HOME/.platformio-xiao-packages"
```

This avoids collisions between the two Arduino framework package lineages in
PlatformIO's shared package cache.

### Build

Debug build:

``` bash
PLATFORMIO_PACKAGES_DIR="$HOME/.platformio-xiao-packages" \
$HOME/.platformio/penv/bin/pio run -e xiao_esp32c6_debug
```

Release build:

``` bash
PLATFORMIO_PACKAGES_DIR="$HOME/.platformio-xiao-packages" \
$HOME/.platformio/penv/bin/pio run -e xiao_esp32c6_release
```

### Upload

Debug upload:

``` bash
PLATFORMIO_PACKAGES_DIR="$HOME/.platformio-xiao-packages" \
$HOME/.platformio/penv/bin/pio run -e xiao_esp32c6_debug \
    -t upload --upload-port /dev/ttyACM0
```

Release upload:

``` bash
PLATFORMIO_PACKAGES_DIR="$HOME/.platformio-xiao-packages" \
$HOME/.platformio/penv/bin/pio run -e xiao_esp32c6_release \
    -t upload --upload-port /dev/ttyACM0
```

`/dev/ttyACM0` is an example from the current WSL setup, not a hard-coded
project setting. The actual device path may differ.

### Serial monitor

Debug serial monitor:

``` bash
PLATFORMIO_PACKAGES_DIR="$HOME/.platformio-xiao-packages" \
$HOME/.platformio/penv/bin/pio device monitor \
    -e xiao_esp32c6_debug --port /dev/ttyACM0
```

The same `/dev/ttyACM0` note applies here: use the actual device path exposed
by the current system.

### Windows, WSL, and native USB note

The XIAO ESP32-C6 uses native USB. Under Windows with WSL, the device may need
to be shared and attached with `usbipd` before upload or serial access. Reset
and deep sleep can cause the native USB device to disconnect and re-enumerate.
`xiao_esp32c6_debug` disables deep sleep specifically to make USB/serial
bring-up easier. `xiao_esp32c6_release` uses normal deep sleep, so serial
monitoring is not the best way to observe repeated wake cycles; server-side
measurement visibility is more useful for that test.

## Native tests

Run the host-side protocol tests separately from hardware build and upload
workflows:

``` bash
$HOME/.platformio/penv/bin/pio test -e native
```

These native tests do not exercise hardware access, Wi-Fi, NVS persistence, or
complete end-to-end firmware behavior.

## Notes

-   Keep the repository in the WSL/Linux filesystem for development
    rather than under `/mnt/c/...`.
-   `usbipd bind` normally persists, while `usbipd attach` normally
    needs to be run again after restarting Windows.
-   Windows owns the physical USB device until it is attached to WSL.
-   When the ESP32 is attached to WSL, use PlatformIO and the serial
    monitor from the WSL environment.
