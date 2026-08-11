# Environment Sensor Firmware

Firmware for the ESP32-based environment sensor.

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

## Opening the project

From the project directory in WSL:

``` bash
code .
```

Verify that VS Code shows `WSL: Ubuntu` as the remote environment.

PlatformIO IDE and the Microsoft C/C++ extension should be installed in
the WSL environment, not only in the local Windows VS Code environment.

## Build, upload and serial monitor

These operations can be run using the PlatformIO controls in VS Code or
from the WSL terminal.

### Build

``` bash
pio run
```

### Upload

``` bash
pio run --target upload
```

### Serial monitor

``` bash
pio device monitor
```

## Notes

-   Keep the repository in the WSL/Linux filesystem for development
    rather than under `/mnt/c/...`.
-   `usbipd bind` normally persists, while `usbipd attach` normally
    needs to be run again after restarting Windows.
-   Windows owns the physical USB device until it is attached to WSL.
-   When the ESP32 is attached to WSL, use PlatformIO and the serial
    monitor from the WSL environment.
