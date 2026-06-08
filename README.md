# ECAN-E02 ESPHome CAN-to-Ethernet firmware

This repository contains ESPHome firmware for the inexpensive ECAN-E02 board, turning it into an Ethernet-connected CAN monitor for Home Assistant.

The target board uses an ESP32-U4WD, an RTL8201 Ethernet PHY, and an isolated CAN transceiver. The confirmed firmware pinout is already encoded in [configs/ecan-e02.yaml](configs/ecan-e02.yaml).

## Current Status

- ESP32 flashing over USB serial is working.
- CAN is confirmed on `GPIO10` TX and `GPIO9` RX.
- CAN self-test passes when the board is powered from its normal 12 V input.
- RTL8201 management and RMII pins are traced and configured with `phy_addr: 0`.
- The main firmware compiles, flashes, and boots cleanly.
- Ethernet link and DHCP still need final validation with the RJ45 port connected to a live network.

The default firmware is conservative: CAN starts in `LISTENONLY` mode, so it can monitor a live CAN bus without acknowledging or transmitting frames.

## What You Need

- ECAN-E02 board.
- 12 V power supply connected to the board power input. USB-only power may not power the isolated CAN side.
- USB-to-serial adapter for flashing. A CH343/CH34x adapter works.
- Ethernet cable connected to a network with DHCP.
- CANH/CANL connected to the bus you want to monitor.
- A Linux machine with `git`, `make`, and `uv`/`uvx` available.

For Linux USB serial support, the in-kernel `ch341` driver is usually enough for WCH CH34x adapters. Some CH343 adapters work better with WCH's vendor driver; this repo includes `scripts/ch343-driver.sh` for building/loading that driver when needed.

## Quick Start

Clone the repository and check that ESPHome can read the main config:

```sh
make config
```

Compile the firmware:

```sh
make compile
```

Find the USB serial port:

```sh
./scripts/usb-check.sh
./scripts/serial-port.sh
```

Flash the board. Replace the port with the one shown on your system:

```sh
make flash PORT=/dev/ttyACM0
```

You can also use the script directly:

```sh
./scripts/flash.sh /dev/ttyACM0
```

Read serial logs after flashing:

```sh
make logs PORT=/dev/ttyACM0
```

Successful boot logs should show the ESP32 starting, CAN configured in listen-only mode, and Ethernet starting with RTL8201 `phy_addr: 0`.

## Home Assistant

After Ethernet obtains an IP address, Home Assistant should be able to discover the node through ESPHome/mDNS as `ecan-e02.local`.

The firmware exposes:

- Device status and restart control.
- Ethernet IP and MAC diagnostics.
- CAN RX frame count.
- Last received CAN frame.
- Optional CAN RX logging switch.
- LINK, ERR, and CAN status LED entities.
- Heap, loop-time, reset-reason, and device-info diagnostics.

Many diagnostic entities are disabled by default in Home Assistant to keep the device quiet. Enable them from the ESPHome device page when you need them.

The current config leaves API encryption and OTA passwords unset for bench bring-up. Add ESPHome API encryption and OTA credentials before putting the device on an untrusted network.

## Configuration

Most normal changes are substitutions at the top of [configs/ecan-e02.yaml](configs/ecan-e02.yaml):

```yaml
substitutions:
  name: ecan-e02
  friendly_name: ECAN E02
  can_bit_rate: 500KBPS
  can_mode: LISTENONLY
```

Use `LISTENONLY` for passive monitoring. Use `NORMAL` only when you want the board to acknowledge frames or transmit on the CAN bus. The included "CAN Test Frame" button only sends when `can_mode` is `NORMAL`.

Common CAN bit rates include `125KBPS`, `250KBPS`, `500KBPS`, and `1000KBPS`. Match the existing bus.

Do not short CANH to CANL for testing. Use a correctly terminated CAN bus; many small bench setups need a 120 ohm resistor across CANH/CANL at the end of the bus.

## Status LEDs

The board LEDs are active-low and are configured as:

| LED label | Firmware behavior |
| --- | --- |
| LINK | On while ESPHome reports Ethernet connected. |
| ERR | ESPHome status LED; blinks for warnings/errors. |
| CAN | Pulses on CAN RX and CAN test TX attempts. |

## Repository Layout

- [configs/ecan-e02.yaml](configs/ecan-e02.yaml): main firmware for normal use.
- [components/ecan_e02/](components/ecan_e02/): local ESPHome helper component with board diagnostics.
- [scripts/](scripts/): user-facing build, flash, log, USB, and driver helpers.
- [dev/](dev/): hardware bring-up notes, debug firmware, probing configs, and development-only scripts.

## Development Notes

The ECAN-E02 pinout and bring-up history live in [dev/README.md](dev/README.md). Those files are useful if you are tracing a board variant, debugging the USB adapter, checking CAN electrically, or validating the RTL8201 PHY. Normal users should start with [configs/ecan-e02.yaml](configs/ecan-e02.yaml).
