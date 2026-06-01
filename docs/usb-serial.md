# USB Serial Requirements

The ECAN-E02 project flashes and logs through an external WCH CH343-family USB-UART adapter. The CH343 adapter is not part of the ECAN-E02 PCB, so USB enumeration problems usually belong to the host cable, host port, or external adapter rather than the board itself.

## Linux Requirements

A Linux host should provide:

- a USB data cable and port that make the adapter visible in `lsusb`
- access to the created serial device, usually through the `dialout` group or a root shell
- a USB serial driver that matches the enumerated USB device

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

## Flashing

Auto-detect the serial port and flash the bare bring-up firmware:

```sh
./scripts/flash-bare.sh
```

If auto-detection picks the wrong device or cannot see the host serial device, pass the port explicitly:

```sh
./scripts/flash-bare.sh /dev/ttyUSB0
./scripts/flash-bare.sh /dev/ttyACM0
./scripts/flash-bare.sh /dev/ttyCH343USB0
```

Read logs the same way:

```sh
./scripts/logs.sh /dev/ttyUSB0
```

## Capturing Attach Events

When changing cable, port, adapter, or driver, capture the kernel and udev attach event:

```sh
./scripts/watch-usb.sh 60
```

Replug the external CH343 adapter while the watcher is running. The useful output is the USB ID, bound driver, and created tty name.

## Container Or Sandbox Note

This workspace may not expose host serial devices inside `/dev`. If the host shell sees `/dev/ttyUSB0`, `/dev/ttyACM0`, or `/dev/ttyCH343USB0` but this workspace does not, pass the device through to the environment or run the flash command from a host shell.
