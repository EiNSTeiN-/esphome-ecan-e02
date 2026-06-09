# USB Serial Requirements

The Ebyte ECAN-E02 project flashes and logs through an external WCH CH343-family USB-UART adapter. The CH343 adapter is not part of the Ebyte ECAN-E02 PCB, so USB enumeration problems usually belong to the host cable, host port, or external adapter rather than the board itself.

## Linux Requirements

A Linux host should provide:

- a USB data cable and port that make the adapter visible in `lsusb`
- access to the created serial device, usually through the `dialout` group or a root shell
- a USB serial driver that matches the enumerated USB device
- optional USB-UART `DTR` and `RTS` lines wired to the ESP32 boot strap and reset pins for automatic flashing

Useful checks:

```sh
lsusb
./scripts/serial-port.sh
./scripts/usb-check.sh
```

Expected serial device names are:

- `/dev/ttyUSB*` for common WCH UART adapters handled by the kernel `ch341` driver
- `/dev/ttyACM*` for CDC ACM devices handled by `cdc_acm`
- `/dev/ttyCH343USB*` for WCH's vendor CH343 driver

## Driver Selection

Choose the driver from the USB ID and the tty device that appears:

| USB ID | Typical tty | Linux driver | Notes |
| --- | --- | --- | --- |
| `1a86:7523`, `1a86:7522`, `1a86:5523` | `/dev/ttyUSB0` | `ch341` | Older WCH/QinHeng CH34x path. |
| `1a86:55d*` | `/dev/ttyACM0` | `cdc_acm` | Newer WCH CH342/CH343/CH910x family in CDC mode. |
| `1a86:55d*` | `/dev/ttyCH343USB0` | WCH vendor `ch343` | Recommended fallback if CDC mode does not create a usable tty. |
| `303a:1001` | `/dev/ttyACM0` | `cdc_acm` | ESP32 native USB JTAG/serial, if available on the target wiring. |

Recommended order on Linux:

1. Confirm the adapter appears in `lsusb`.
2. Try the kernel drivers first:

   ```sh
   sudo modprobe cdc_acm
   sudo modprobe ch341
   ```

3. If a WCH `1a86:55d*` device appears but no stable tty is created, use WCH's CH343-family vendor driver:

   ```sh
   ./scripts/ch343-driver.sh build
   ./scripts/ch343-driver.sh load
   ./scripts/ch343-driver.sh status
   ```

4. If the vendor driver should persist across reboots, install it:

   ```sh
   ./scripts/ch343-driver.sh install
   ```

Only use the vendor CH343 driver after `lsusb` confirms a WCH `1a86:*` device. A missing driver normally prevents tty creation, but the USB device should still appear in `lsusb`.

## Auto Bootloader Wiring

For automatic ESP32 bootloader entry, wire the USB-UART control lines to the ESP32 strap/reset pins:

| USB-UART signal | ESP32 signal | Purpose |
| --- | --- | --- |
| `DTR` | `GPIO0` / `BOOT` | Pull low during reset to enter the ROM serial bootloader. |
| `RTS` | `EN` / `CHIP_PU` / `RESET` | Reset the ESP32. |

For bench bring-up on an unknown board, add about 1k series resistance on each control line. For a permanent design, use the normal ESP32 auto-reset transistor circuit rather than direct wiring.

Direct wiring can confuse generic serial terminals because some of them assert `DTR` or `RTS` when opening the port. If logs are needed with direct wiring, use this project's reset-log helper:

```sh
./dev/scripts/serial-reset-log.sh /dev/ttyACM0
```

## Flashing

Auto-detect the serial port and flash the bare bring-up firmware:

```sh
./dev/scripts/flash-bare.sh
```

If auto-detection picks the wrong device or cannot see the host serial device, pass the port explicitly:

```sh
./dev/scripts/flash-bare.sh /dev/ttyUSB0
./dev/scripts/flash-bare.sh /dev/ttyACM0
./dev/scripts/flash-bare.sh /dev/ttyCH343USB0
```

Read logs the same way:

```sh
./scripts/logs.sh /dev/ttyUSB0 dev/configs/ecan-e02-bare.yaml
```

For direct `DTR`/`RTS` wiring, prefer:

```sh
./dev/scripts/serial-reset-log.sh /dev/ttyUSB0
```

## Capturing Attach Events

When changing cable, port, adapter, or driver, capture the kernel and udev attach event:

```sh
./dev/scripts/watch-usb.sh 60
```

Replug the external CH343 adapter while the watcher is running. The useful output is the USB ID, bound driver, and created tty name.

## Container Or Sandbox Note

This workspace may not expose host serial devices inside `/dev`. If the host shell sees `/dev/ttyUSB0`, `/dev/ttyACM0`, or `/dev/ttyCH343USB0` but this workspace does not, pass the device through to the environment or run the flash command from a host shell.
