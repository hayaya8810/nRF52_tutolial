# Linux Mint + nRF52-DK + nRF Connect SDK Development Environment Setup Guide

[TOC]

## Purpose

Create an environment on Linux Mint that can develop, build, and flash this repository's Zephyr/NCS application using a Nordic nRF52-DK.

This guide targets the following.

- OS: Linux Mint
- SDK: nRF Connect SDK
- NCS version: `v3.3.0`
- Board: nRF52-DK
- SoC: nRF52832
- Zephyr board target: `nrf52dk/nrf52832`
- Build tool: `west`
- Application: `blinky`

This project does not only use Zephyr directly; it also uses Nordic UART Service, NUS, which is included in Nordic's `nrf` module.  
For that reason, the development environment assumes **nRF Connect SDK, NCS**, Nordic's official SDK that includes Zephyr, rather than upstream Zephyr alone.

---

## 0. Environment Verified on This PC

This guide was verified in the following environment.

| Item | Version / Path |
| --- | --- |
| NCS | `3.3.0` |
| NCS workspace | `/home/my_home/ncs/v3.3.0` |
| Zephyr | `/home/my_home/ncs/v3.3.0/zephyr` |
| west | `1.5.0` |
| Python | `3.12.3` |
| CMake | `3.28.1` |
| Ninja | `1.11.1` |
| Devicetree compiler | `DTC 1.7.0` |
| Zephyr SDK | `0.17.0` |
| ARM GCC | `arm-zephyr-eabi-gcc 12.2.0` |
| nrfjprog | `10.24.2 external` |
| J-Link | `JLinkARM.dll 9.40` |

Verification commands:

```bash
west topdir
west --version
python3 --version
cmake --version
ninja --version
dtc --version
nrfjprog --version
```

Notes:

- The Zephyr SDK installed with NCS has been verified as `0.17.0` in this environment.
- The ARM GCC installed with NCS has been verified as `12.2.0` in this environment.
- The Python installed with NCS has been verified as `3.12.3` in this environment.
- The `west` used from PATH is `/home/my_home/.local/bin/west`.
- `nrfutil` exists inside the NCS toolchain, but it may not be in the normal terminal PATH. In that case, use `west flash -r nrfjprog`.

---

## 1. Relationship Between NCS and Zephyr

NCS stands for nRF Connect SDK, an SDK for the nRF series provided by Nordic Semiconductor.

Zephyr is included inside NCS.

```text
/home/my_home/ncs/v3.3.0/
├── zephyr/    # Zephyr itself
├── nrf/       # Nordic additional modules
├── modules/   # HAL and external modules
└── ...
```

This project enables the following in [prj.conf](../prj.conf).

```conf
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_NUS=y
```

`CONFIG_BT_NUS` configures the use of Nordic UART Service, so this project assumes NCS.

---

## 2. Install OS Packages

Install the minimum required packages on Linux.

```bash
sudo apt update
sudo apt upgrade

sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget python3-dev python3-venv python3-tk \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1 \
  minicom
```

Verify:

```bash
cmake --version
python3 --version
ninja --version
dtc --version
```

The following versions have been verified on this PC.

```text
CMake 3.28.1
Python 3.12.3
Ninja 1.11.1
DTC 1.7.0
```

---

## 3. Install NCS

On this PC, NCS is placed here.

```text
/home/my_home/ncs/v3.3.0
```

When creating a new environment, install Nordic's nRF Connect SDK `v3.3.0` and its toolchain.  
After installation, the layout should look like this.

```text
/home/<user-name>/ncs/v3.3.0/
├── zephyr/
├── nrf/
├── modules/
└── ...

/home/<user-name>/ncs/toolchains/<toolchain-id>/
├── opt/zephyr-sdk/
├── nrfutil/
└── ...
```

The toolchain on this PC is here.

```text
/home/my_home/ncs/toolchains/911f4c5c26
```

Notes:

- In this environment, Zephyr SDK `0.17.0` is included in the NCS toolchain.
- This is not a setup where a separate `~/zephyr-sdk-*` is manually installed.
- The upstream-Zephyr `~/zephyrproject` layout is not assumed for this project.

---

## 4. Check the west Workspace

For this project's build, the west workspace must point to NCS.

```bash
cd /home/my_home/workspace/blinky
west topdir
```

Expected value:

```text
/home/my_home/ncs/v3.3.0
```

Also check the west settings.

```bash
west config --list
```

The settings on this PC are as follows.

```text
manifest.path=nrf
manifest.file=west.yml
zephyr.base=zephyr
```

---

## 5. Get This Project

On this PC, the application repository is placed here.

```text
/home/my_home/workspace/blinky
```

Example for cloning it into a new environment:

```bash
mkdir -p ~/workspace
cd ~/workspace
git clone <this-repository-url> blinky
cd blinky
```

When cloning from GitHub, the following error may appear.

```console
remote: Invalid username or token. Password authentication is not supported for Git operations.
fatal: Authentication failed
```

GitHub does not support HTTPS password authentication, so use one of the following.

- HTTPS + Personal Access Token
- SSH key

---

## 6. Project Structure

This repository is built as a Zephyr application.

```text
blinky/
├── CMakeLists.txt
├── prj.conf
├── sample.yaml
├── boards/
└── src/
    ├── main.c
    ├── button.c
    ├── button.h
    ├── led.c
    ├── led.h
    ├── nus.c
    └── nus.h
```

Main roles:

| File | Role |
|---|---|
| `src/main.c` | Initialization, button callback registration, main loop |
| `src/button.c/h` | nRF52-DK 4-button input, GPIO interrupts, debounce |
| `src/led.c/h` | nRF52-DK 4-LED control, ON/OFF/Toggle/Blink |
| `src/nus.c/h` | BLE Peripheral, Nordic UART Service, LED state notification |
| `prj.conf` | Kconfig settings for GPIO, Bluetooth, and NUS |
| `CMakeLists.txt` | Adds `main.c`, `button.c`, `led.c`, and `nus.c` as build targets |

Current `prj.conf`:

```conf
CONFIG_GPIO=y

CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_DEVICE_NAME="Blinky_NUS"
CONFIG_BT_MAX_CONN=1
CONFIG_BT_NUS=y
```

---

## 7. Application Behavior

This application behaves as follows on the nRF52-DK.

- Button 1: starts/stops LED 1 blinking with a 1000 ms period
- Button 2: starts/stops LED 2 blinking with a 500 ms period
- Button 3: starts/stops LED 3 blinking with a 250 ms period
- Button 4: toggles LED 4 ON/OFF
- Advertises as a BLE Peripheral with the name `Blinky_NUS`
- Sends the LED state to a Central that has enabled NUS notifications

LED blinking is contained inside `src/led.c` using `k_work_delayable`.  
`main.c` only calls `led_blink_toggle()` and `led_toggle()`.

LED state is handled as a bit mask.

| bit | Meaning |
|---|---|
| bit0 | LED 1 |
| bit1 | LED 2 |
| bit2 | LED 3 |
| bit3 | LED 4 |

Examples:

```text
0x00 = all OFF
0x01 = LED1 ON
0x02 = LED2 ON
0x04 = LED3 ON
0x08 = LED4 ON
0x0F = LED1 through LED4 all ON
```

The current NUS payload is sent in an easy-to-check text format, not as 1 byte of raw data.

Examples:

```text
B0 L=1
N3 L=5
```

When the specification with the Central side is fixed in the future, the payload may be changed to a 1 byte bit mask.

---

## 8. Build

Run the following in this project's directory.

```bash
cd /home/my_home/workspace/blinky
west build -b nrf52dk/nrf52832 . -p always
```

If the build succeeds, memory usage is shown at the end.

Build success has been verified on this PC.

```text
Memory region         Used Size  Region Size  %age Used
           FLASH:      108228 B       512 KB     20.64%
             RAM:       20544 B        64 KB     31.35%
```

Notes:

- In this environment, NCS `v3.3.0` sysbuild is used.
- A Partition Manager deprecated warning appears during the build, but the application itself builds successfully.

---

## 9. Connect the nRF52-DK

Connect the nRF52-DK to the PC by USB.

Check the serial port.

```bash
ls /dev/ttyACM*
```

Example:

```text
/dev/ttyACM0
```

To view serial output:

```bash
minicom -D /dev/ttyACM0 -b 115200
```

To exit, press `Ctrl-A` and then `X`.

---

## 10. Flash

First try a normal flash.

```bash
cd /home/my_home/workspace/blinky
west flash
```

In this PC's build context, the default runner is `nrfutil`.

However, if `nrfutil` is not in the PATH in a normal terminal, explicitly specify the `nrfjprog` runner.

```bash
west flash -r nrfjprog
```

The following are installed on this PC.

```text
nrfjprog 10.24.2 external
JLinkARM.dll 9.40
```

If multiple J-Link / nRF52-DK devices are connected, specify the device ID.

```bash
west flash -r nrfjprog --dev-id <J-Link serial number>
```

---

## 11. Flash Troubleshooting

### `nrfutil` Is Not Found by `west flash`

`nrfutil` exists inside the NCS toolchain, but it may not be in the PATH of a normal terminal.

Workaround:

```bash
west flash -r nrfjprog
```

Alternatively, enable the NCS toolchain environment variables before running the command.

On this PC, `nrfutil` is here.

```text
/home/my_home/ncs/toolchains/911f4c5c26/nrfutil/bin/nrfutil
```

### J-Link Permission Error

udev rules may be missing.  
Install the udev rules when installing SEGGER J-Link Software or Nordic Command Line Tools.

After applying them, reload udev and reconnect the USB cable.

```bash
sudo udevadm control --reload
sudo udevadm trigger
```

### Multiple Boards Are Connected and the Flash Target Cannot Be Selected

Specify the J-Link serial number.

```bash
west flash -r nrfjprog --dev-id <J-Link serial number>
```

---

## 12. Operation Check

After flashing, check the serial log.

```bash
minicom -D /dev/ttyACM0 -b 115200
```

Expected log example:

```text
BLE advertising as Blinky_NUS
```

Button operation:

- Press Button 1 to start/stop LED 1 blinking with a 1000 ms period
- Press Button 2 to start/stop LED 2 blinking with a 500 ms period
- Press Button 3 to start/stop LED 3 blinking with a 250 ms period
- Press Button 4 to turn LED 4 ON/OFF

Connect to `Blinky_NUS` from a BLE Central and enable notification on the NUS TX characteristic. The LED state will then be notified.

---

## 13. Frequently Used Commands

```bash
# Check the NCS workspace
west topdir

# west version
west --version

# Build
cd /home/my_home/workspace/blinky
west build -b nrf52dk/nrf52832 . -p always

# Flash
west flash

# Flash using the nrfjprog runner
west flash -r nrfjprog

# Serial log
minicom -D /dev/ttyACM0 -b 115200

# Check nRF52-DK in the board list
west boards | rg '^nrf52dk'
```

---

## 14. Development Notes

In this project, responsibilities are separated by feature.

- Keep GPIO and interrupt details inside `button.c`
- Keep LED state management and blinking logic inside `led.c`
- Keep BLE/NUS connection management and notification handling inside `nus.c`
- Keep `main.c` focused on initialization and application flow

Possible future changes:

1. Change the NUS payload from text format to a 1 byte bit mask
2. Clarify the specification for immediately notifying the current LED state when BLE connects
3. Decide whether the state during LED blinking should be treated as the "current physical output" or the "blinking enabled state"
4. Fix the receive specification for the Central-side application

---

## 15. Tips

### Check Kconfig

Use `menuconfig` if you want to view or temporarily change Kconfig settings on screen.

```bash
cd /home/my_home/workspace/blinky
west build -t menuconfig
```

If you want a GUI, `guiconfig` can also be used.

```bash
west build -t guiconfig
```

`guiconfig` may require `python3-tk`.  
Changes made here are only reflected in the build directory, so write settings that should be permanent back to `prj.conf`.

### Check the Final Kconfig

The final Kconfig after build is output to `.config`.

```bash
rg CONFIG_BT_NUS build/blinky/zephyr/.config
rg CONFIG_BT_DEVICE_NAME build/blinky/zephyr/.config
```

You can check whether settings written in `prj.conf` are enabled and whether they have been disabled by other dependencies.

### Check the Final Devicetree

The final Devicetree after build is output to `zephyr.dts`.

```bash
rg 'led0|led1|led2|led3' build/blinky/zephyr/zephyr.dts
rg 'sw0|sw1|sw2|sw3' build/blinky/zephyr/zephyr.dts
```

You can check which actual GPIO pins the LED and Button aliases resolve to.  
If GPIO is not behaving as expected, this is a good first place to look.

### Clean the build Directory Later

To return an already-built directory to a pristine state:

```bash
west build -t pristine
```

For normal use, you can also add `-p always` as follows to perform a clean build.

```bash
west build -b nrf52dk/nrf52832 . -p always
```

### Check the flash Runner

Check which runner `west flash` uses and which runners are available.

```bash
west flash --context
```

On this PC, the default runner is `nrfutil`.  
If `nrfutil` is not in the PATH or does not work correctly, explicitly specify a runner.

```bash
west flash -r nrfjprog
west flash -r jlink
```

### Use nRF Connect for Mobile to Check BLE

nRF Connect for Mobile, a smartphone app, is useful for checking the BLE Central side.

Check procedure:

1. Flash the nRF52-DK
2. Scan with nRF Connect for Mobile
3. Find and connect to `Blinky_NUS`
4. Enable notification on the Nordic UART Service TX characteristic
5. Press the nRF52-DK buttons and check the notifications

Before creating a PC-side Central application, this lets you isolate whether the Peripheral and NUS notifications are working.

### Serial Log Is Not Visible

The serial port is not always `/dev/ttyACM0`.

```bash
ls /dev/ttyACM*
```

Unplug and reconnect the nRF52-DK, and check which port appeared or disappeared.  
If multiple USB serial devices are connected, it may be `/dev/ttyACM1` or similar.

Connection example:

```bash
minicom -D /dev/ttyACM0 -b 115200
```

If no log appears, check whether the baudrate is `115200` and whether a different `/dev/ttyACM*` port is open.

### printf / printk Output Is Not Visible

This project outputs logs with `printf()`.  
If nothing appears on serial, check whether UART console is enabled and whether the correct port is open.

If needed, add console and logging settings to `prj.conf`.

Example:

```conf
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=y
CONFIG_PRINTK=y
```

### Decide the NUS Payload Format

The current `nus.c` sends an easy-to-check text format.

Examples:

```text
B0 L=1
N3 L=5
```

When fixing the specification with the Central side, decide which of the following to use.

- Continue using the text format, which is easy to debug
- Change to a 1 byte bit mask, which uses less communication data

Example for a 1 byte bit mask:

```text
bit0 = LED1
bit1 = LED2
bit2 = LED3
bit3 = LED4
```

---

## References

- nRF Connect SDK documentation
- nRF52-DK board documentation
- Zephyr documentation
- Nordic Command Line Tools documentation
