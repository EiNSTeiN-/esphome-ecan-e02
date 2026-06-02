# ECAN-E02 ESPHome bring-up

This workspace is set up for an ECAN-E02 board with:

- ESP32-U4WD flash target, treated as a classic ESP32/`esp32dev` in ESPHome
- external CH343 USB-UART adapter for flashing/logging
- likely RTL8201 RMII Ethernet PHY
- onboard CAN transceiver, pins still to be confirmed

The first step is intentionally small: flash a bare ESP32 ESPHome image over USB, verify serial logs, then trace the PHY and CAN wiring before enabling bus drivers.

## Quick start

Check what USB serial device is visible:

```sh
./scripts/usb-check.sh
```

To watch a replug event:

```sh
./scripts/watch-usb.sh 60
```

Check the config:

```sh
./scripts/esphome.sh config configs/ecan-e02-bare.yaml
```

Compile:

```sh
./scripts/esphome.sh compile configs/ecan-e02-bare.yaml
```

Flash over the detected serial interface:

```sh
./scripts/flash-bare.sh
```

If the port auto-detect does not find the adapter, pass it explicitly. Use `/dev/ttyUSB0` for the external WCH CH34x/CH343 UART path, `/dev/ttyACM0` for an Espressif native USB CDC/JTAG path, or `/dev/ttyCH343USB0` when using WCH's vendor CH343 driver:

```sh
./scripts/flash-bare.sh /dev/ttyUSB0
./scripts/flash-bare.sh /dev/ttyACM0
./scripts/flash-bare.sh /dev/ttyCH343USB0
```

Read serial logs:

```sh
./scripts/logs.sh
```

If `DTR` and `RTS` are wired directly from the USB-UART adapter to `GPIO0/BOOT` and `EN/CHIP_PU`, use the reset-log helper instead. It opens the port with both control lines inactive, pulses reset, and captures boot logs:

```sh
./scripts/serial-reset-log.sh /dev/ttyACM0
```

The bare firmware enables only serial logging and the local `ecan_e02` component. It does not need WiFi secrets and is the safest target while the board pinout is unknown.

## Useful files

- `configs/ecan-e02-bare.yaml`: first flash target
- `configs/ecan-e02-wifi.yaml.example`: optional WiFi/API/OTA layer once serial flashing works
- `configs/ecan-e02-gpio-probe.yaml.example`: passive GPIO input sampler for suspected pins
- `configs/ecan-e02-can-listen.yaml`: traced CAN listen-only config, `GPIO10` TX and `GPIO9` RX through the TPT7721 isolator
- `configs/ecan-e02-can-listen.yaml.example`: ESP32 TWAI/CAN listen-only skeleton
- `configs/ecan-e02-ethernet-rtl8201.yaml.example`: likely RTL8201 RMII skeleton
- `scripts/ch343-driver.sh`: fetch/build/load the WCH CH343-family vendor driver
- `scripts/serial-reset-log.sh`: serial log capture for direct DTR/RTS reset wiring
- `docs/pin-tracing.md`: physical tracing checklist
- `docs/usb-serial.md`: CH343, `cdc_acm`, `ch341`, and Espressif native USB notes

## Expected next steps

1. Flash `configs/ecan-e02-bare.yaml` and confirm serial logs show the `ecan_e02` component.
2. Trace CAN transceiver TXD/RXD to ESP32 GPIOs.
3. Trace RTL8201 MDC, MDIO, REF_CLK, power/reset, and PHY address straps.
4. Copy an example YAML to a real config, fill in confirmed pins, and validate with `./scripts/esphome.sh config`.

## Verification

The following were validated with ESPHome 2026.5.1:

- `configs/ecan-e02-bare.yaml`: config and compile
- `configs/ecan-e02-can-listen.yaml`: config
- `configs/ecan-e02-can-listen.yaml.example`: config and compile
- `configs/ecan-e02-ethernet-rtl8201.yaml.example`: config and compile
- `configs/ecan-e02-gpio-probe.yaml.example`: config
- bare firmware flash and boot logs on the ESP32-U4WD target over CH343 CDC ACM; esptool reports this target as `ESP32-U4WDH`

The bare serial-only config remains the safest first flash target. Compile the specific config you intend to upload before flashing.

## ESPHome notes

This project uses ESPHome through `uvx`:

```sh
env UV_CACHE_DIR=/tmp/uv-cache UV_TOOL_DIR=/tmp/uv-tools uvx --from esphome esphome version
```

The helper scripts set those environment variables automatically.

## Git in this workspace

This environment has a read-only `.git` placeholder directory, so this checkout uses `.git-local` as the real Git directory. Use the wrapper for local Git commands:

```sh
./scripts/git.sh status
./scripts/git.sh log --oneline
```

The ESP32 classic RMII data pins are fixed in ESPHome/ESP-IDF:

| ESP32 GPIO | RMII signal |
| --- | --- |
| GPIO19 | TXD0 |
| GPIO21 | TX_EN |
| GPIO22 | TXD1 |
| GPIO25 | RXD0 |
| GPIO26 | RXD1 |
| GPIO27 | CRS_DV |

The pins still worth tracing are usually `MDC`, `MDIO`, `REF_CLK`, PHY address straps, reset/power enable, and the CAN transceiver TXD/RXD pair.
