# ECAN-E02 ESPHome bring-up

This workspace is set up for an ECAN-E02 board with:

- ESP32-U4WD flash target, treated as a classic ESP32/`esp32dev` in ESPHome
- CH343 USB serial bridge
- likely RTL8201 RMII Ethernet PHY
- onboard CAN transceiver, pins still to be confirmed

The first step is intentionally small: flash a bare ESP32 ESPHome image over USB, verify serial logs, then trace the PHY and CAN wiring before enabling bus drivers.

## Quick start

Check the config:

```sh
./scripts/esphome.sh config configs/ecan-e02-bare.yaml
```

Compile:

```sh
./scripts/esphome.sh compile configs/ecan-e02-bare.yaml
```

Flash over the CH343 serial adapter:

```sh
./scripts/flash-bare.sh
```

If the port auto-detect does not find the CH343, pass it explicitly:

```sh
./scripts/flash-bare.sh /dev/ttyUSB0
```

Read serial logs:

```sh
./scripts/logs.sh
```

The bare firmware enables only serial logging and the local `ecan_e02` component. It does not need WiFi secrets and is the safest target while the board pinout is unknown.

## Useful files

- `configs/ecan-e02-bare.yaml`: first flash target
- `configs/ecan-e02-wifi.yaml.example`: optional WiFi/API/OTA layer once serial flashing works
- `configs/ecan-e02-gpio-probe.yaml.example`: passive GPIO input sampler for suspected pins
- `configs/ecan-e02-can-listen.yaml.example`: ESP32 TWAI/CAN listen-only skeleton
- `configs/ecan-e02-ethernet-rtl8201.yaml.example`: likely RTL8201 RMII skeleton
- `docs/pin-tracing.md`: physical tracing checklist

## Expected next steps

1. Flash `configs/ecan-e02-bare.yaml` and confirm serial logs show the `ecan_e02` component.
2. Trace CAN transceiver TXD/RXD to ESP32 GPIOs.
3. Trace RTL8201 MDC, MDIO, REF_CLK, power/reset, and PHY address straps.
4. Copy an example YAML to a real config, fill in confirmed pins, and validate with `./scripts/esphome.sh config`.

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
