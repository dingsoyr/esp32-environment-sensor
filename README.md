# Environment Sensor Firmware

Firmware for the ESP32-based environment sensor.

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

The project currently defines three PlatformIO environments:

-   `debug`: ESP32 development build with debug logging enabled.
-   `release`: ESP32 build with normal debug logging disabled.
-   `native`: host-side tests of the API response/protocol parser only.

The `native` environment does not exercise full firmware behavior, Wi-Fi,
storage, hardware access, or end-to-end server integration.

Buffered measurements are persisted in NVS as raw `Measurement` records. That
record now includes `battery_voltage` and `battery_percent`, and those values
are uploaded inside each measurement object. The current firmware uses
temporary deterministic dummy battery values pending real hardware battery
sensing.

## Build, upload and serial monitor

These operations can be run using the PlatformIO controls in VS Code or
from the WSL terminal.

The short `pio ...` commands below assume the PlatformIO CLI is already
available on `PATH`. If it is not, use the explicit PlatformIO venv path shown
in the alternate examples.

### Build

``` bash
pio run -e debug
```

Alternate form:

``` bash
$HOME/.platformio/penv/bin/pio run -e debug
```

### Release build

``` bash
pio run -e release
```

Alternate form:

``` bash
$HOME/.platformio/penv/bin/pio run -e release
```

### Upload

``` bash
pio run -e debug -t upload
```

Alternate form:

``` bash
$HOME/.platformio/penv/bin/pio run -e debug -t upload
```

### Serial monitor

``` bash
pio device monitor
```

Alternate form:

``` bash
$HOME/.platformio/penv/bin/pio device monitor
```

### Native tests

``` bash
pio test -e native
```

Alternate form:

``` bash
$HOME/.platformio/penv/bin/pio test -e native
```

These native tests currently cover the API response parser only.

## Notes

-   Keep the repository in the WSL/Linux filesystem for development
    rather than under `/mnt/c/...`.
-   `usbipd bind` normally persists, while `usbipd attach` normally
    needs to be run again after restarting Windows.
-   Windows owns the physical USB device until it is attached to WSL.
-   When the ESP32 is attached to WSL, use PlatformIO and the serial
    monitor from the WSL environment.
