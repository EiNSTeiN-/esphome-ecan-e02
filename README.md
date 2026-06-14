# Ebyte ECAN-E02 ESPHome CAN-to-Ethernet firmware

This repository contains ESPHome firmware for the inexpensive Ebyte ECAN-E02 board, turning it into an Ethernet-connected CAN monitor for Home Assistant.

Vendor product page: [Ebyte ECAN-E02](https://www.cdebyte.com/products/ECAN-E02). The board can likely also be purchased through marketplace listings on AliExpress or Amazon.

The target board uses an ESP32-U4WD, an RTL8201 Ethernet PHY, and an isolated CAN transceiver. The confirmed firmware pinout is encoded in [packages/ecan-e02-base.yaml](packages/ecan-e02-base.yaml) and exposed through the user-facing configs in [configs/](configs/).

## Current Status

- ESP32 flashing over USB serial is working.
- CAN is confirmed on `GPIO10` TX and `GPIO9` RX.
- CAN self-test passes when the board is powered from its normal 12 V input.
- RTL8201 management and RMII pins are traced and configured with `phy_addr: 0`.
- The main firmware compiles, flashes, and boots cleanly.
- Ethernet link, DHCP, ESPHome API, and web-server OTA are verified on a live network.

The default firmware is conservative: CAN starts in `LISTENONLY` mode, so it can monitor a live CAN bus without acknowledging or transmitting frames.

## What You Need

- Ebyte ECAN-E02 board.
- 12 V power supply connected to the board power input. USB-only power may not power the isolated CAN side.
- USB-to-serial adapter for flashing. A CH343/CH34x adapter works.
- Ethernet cable connected to a network with DHCP.
- CANH/CANL connected to the bus you want to monitor.
- A Linux machine with `git`, `make`, and `uv`/`uvx` available.

For Linux USB serial support, the in-kernel `ch341` driver is usually enough for WCH CH34x adapters. Some CH343 adapters work better with WCH's vendor driver; this repo includes [scripts/ch343-driver.sh](scripts/ch343-driver.sh) for building/loading that driver when needed.

## Flashing using CH343 USB-TTL

The Ebyte ECAN-E02 does not expose USB directly. To flash it, open the plastic enclosure and solder temporary wires to the programming pads on the underside of the board.

The pads are labeled:

| Ebyte ECAN-E02 pad | CH343 USB-TTL connection |
| --- | --- |
| `3V3` | Adapter `3V3`, not `5V` |
| `GND` | Adapter `GND` |
| `TXD` | Adapter `RXD` |
| `RXD` | Adapter `TXD` |
| `RST` | Adapter `RTS` if available, or a jumper/button to `GND` |
| `BOOT` | Adapter `DTR` if available, or a jumper/button to `GND` |

Use a 3.3 V USB-TTL adapter. Do not connect a 5 V UART signal to the ESP32 pads.

The `RST` and `BOOT` wires are optional. Without them, put the ESP32 into the serial bootloader manually with the usual ESP32 sequence:

1. Hold or jumper `BOOT` to `GND`.
2. Reset the board with `RST`, or briefly remove and restore power.
3. Release `BOOT`.
4. Run the flash command.

For normal firmware operation and CAN validation, power the Ebyte ECAN-E02 from its 12 V input. USB-TTL power alone may be enough for flashing the ESP32 side, but it does not power the isolated CAN side.

### Backing up the factory firmware

Before replacing the vendor firmware, make a local flash backup. This gives you a recovery image if you want to return a board to its original state. Keep the backup private unless Ebyte explicitly allows redistribution.

Use a recent `esptool`; version 5.3.0 has been tested with the ESP32-U4WD/U4WDH embedded flash on this board. Older `esptool` releases may connect to the ROM bootloader but fail after uploading the flasher stub.

First confirm that the adapter can reset the board and detect the embedded flash:

```sh
PORT=/dev/ttyACM0

UV_CACHE_DIR=/tmp/uv-cache UV_TOOL_DIR=/tmp/uv-tools \
  uvx --from esptool esptool \
  --chip esp32 \
  --port "$PORT" \
  --baud 115200 \
  --before default-reset \
  --after no-reset \
  flash-id
```

If `RST` and `BOOT` are not connected to `RTS` and `DTR`, put the board into the ESP32 bootloader manually before running the command and replace `--before default-reset` with `--before no-reset`.

Expected output should identify an ESP32-U4WD or ESP32-U4WDH and detect a 4 MB flash. Then read the full flash:

```sh
mkdir -p dev/factory-dumps

UV_CACHE_DIR=/tmp/uv-cache UV_TOOL_DIR=/tmp/uv-tools \
  uvx --from esptool esptool \
  --chip esp32 \
  --port "$PORT" \
  --baud 460800 \
  --before default-reset \
  --after no-reset \
  read-flash 0x0 0x400000 dev/factory-dumps/ecan-e02-factory-backup.bin
```

The backup should be exactly `4194304` bytes. A quick sanity check is to look for the ESP32 bootloader image at offset `0x1000`, the partition table at `0x8000`, and the application image at `0x10000`:

```sh
stat -c '%n %s bytes' dev/factory-dumps/ecan-e02-factory-backup.bin
xxd -s 0x1000 -l 16 dev/factory-dumps/ecan-e02-factory-backup.bin
xxd -s 0x8000 -l 16 dev/factory-dumps/ecan-e02-factory-backup.bin
xxd -s 0x10000 -l 16 dev/factory-dumps/ecan-e02-factory-backup.bin
```

After the backup, reset the board normally before using the factory firmware again:

```sh
./dev/scripts/serial-reset-log.sh "$PORT" 6
```

If reset control is not wired, briefly pull `RST` to `GND` or cycle board power instead.

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

## Home Assistant and ESPHome Builder

The firmware includes ESPHome project metadata and a `dashboard_import` URL:

```yaml
dashboard_import:
  package_import_url: github://EiNSTeiN-/esphome-ecan-e02/configs/ecan-e02.factory.yaml@main
  import_full_config: false
```

After Ethernet obtains an IP address, Home Assistant should discover the device through ESPHome/mDNS. The firmware uses `name_add_mac_suffix: true`, so the hostname will be similar to `ecan-e02-1a2b3c.local`, not plain `ecan-e02.local`.

The ESPHome Builder dashboard should show the device as adoptable. Taking control imports the public factory config from this repository, which in turn loads the reusable package from [packages/ecan-e02-base.yaml](packages/ecan-e02-base.yaml). Users can then override substitutions such as CAN bit rate in their own ESPHome YAML while still receiving upstream package updates.

If discovery does not appear, add the ESPHome integration manually in Home Assistant using the device IP address or the MAC-suffixed `.local` hostname.

The base firmware enables ESPHome native OTA on port `3232`, used by ESPHome Builder and `esphome upload` by default. It does not enable the browser web UI or browser OTA path unless you explicitly add the optional web package.

To add the ESPHome web UI to your own config:

```yaml
packages:
  ecan_e02: github://EiNSTeiN-/esphome-ecan-e02/packages/ecan-e02-base.yaml@main
  web: github://EiNSTeiN-/esphome-ecan-e02/packages/ecan-e02-web.yaml@main
```

The web UI can upload `firmware.bin` or `firmware.ota.bin` from `http://<device-ip>/`. Do not upload `firmware.factory.bin` through OTA.

The firmware exposes:

- Device status and restart control.
- Ethernet IP and MAC diagnostics.
- CAN RX frame count.
- Last received CAN frame.
- Optional CAN RX logging switch.
- LINK, ERR, and CAN status LED entities.
- Heap, loop-time, reset-reason, and device-info diagnostics.

Many diagnostic entities are disabled by default in Home Assistant to keep the device quiet. Enable them from the ESPHome device page when you need them.

The public bring-up firmware leaves API encryption and native OTA passwords unset so first-time adoption works without per-user secrets baked into the firmware. After adoption, add ESPHome API encryption and OTA credentials in your own ESPHome config before putting the device on an untrusted network. If you enable the optional web package, secure or disable the web UI according to your network requirements.

## Configuration

The normal local build config is [configs/ecan-e02.yaml](configs/ecan-e02.yaml). It uses the local component source from [components/ecan_e02/](components/ecan_e02/) so development changes can be compiled immediately.

The public adoption config is [configs/ecan-e02.factory.yaml](configs/ecan-e02.factory.yaml). It fetches both the reusable package and the `ecan_e02` external component from GitHub, so ESPHome Builder can import it after this repository is public.

Most normal changes are substitutions:

```yaml
substitutions:
  name: ecan-e02
  friendly_name: Ebyte ECAN E02
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

- [configs/ecan-e02.yaml](configs/ecan-e02.yaml): local development and flashing config.
- [configs/ecan-e02.factory.yaml](configs/ecan-e02.factory.yaml): public ESPHome Builder adoption config.
- [configs/ecan-e02-web.yaml](configs/ecan-e02-web.yaml): local build config with the optional ESPHome web UI enabled.
- [packages/ecan-e02-base.yaml](packages/ecan-e02-base.yaml): reusable Ebyte ECAN-E02 firmware package.
- [packages/ecan-e02-web.yaml](packages/ecan-e02-web.yaml): optional browser UI and browser OTA package.
- [components/ecan_e02/](components/ecan_e02/): ESPHome helper component with board diagnostics.
- [scripts/](scripts/): user-facing build, flash, log, USB, and driver helpers.
- [dev/](dev/): hardware bring-up notes, debug firmware, probing configs, and development-only scripts.

## Development Notes

The Ebyte ECAN-E02 pinout and bring-up history live in [dev/README.md](dev/README.md). Those files are useful if you are tracing a board variant, debugging the USB adapter, checking CAN electrically, or validating the RTL8201 PHY. Normal users should start with [configs/ecan-e02.yaml](configs/ecan-e02.yaml).
