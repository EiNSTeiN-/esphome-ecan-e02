# Ebyte ECAN-E02 Development Notes

This directory contains the hardware bring-up material for the Ebyte ECAN-E02 ESPHome firmware. It is useful when tracing a board variant, validating the USB serial path, checking CAN electrically, or debugging the RTL8201 Ethernet PHY.

For normal flashing and Home Assistant use, start with the top-level [README.md](../README.md) and [configs/ecan-e02.yaml](../configs/ecan-e02.yaml).

## Layout

- [configs/](configs/): bare firmware, CAN probes, LED tests, MDIO scanner, and external CAN peer configs.
- [datasheets/](datasheets/README.md): local PDFs for confirmed ICs, likely ICs, and bench hardware.
- [docs/pin-tracing.md](docs/pin-tracing.md): confirmed ESP32, CAN, LED, and RTL8201 pin tracing notes.
- [manuals/](manuals/README.md): local vendor manuals used for factory-firmware behavior checks.
- [docs/usb-serial.md](docs/usb-serial.md): CH343/CH34x and bootloader serial notes.
- [docs/external-can-peer.md](docs/external-can-peer.md): ESP32-S3-Zero plus VP230 external CAN peer test setup.
- [scripts/flash-bare.sh](scripts/flash-bare.sh): flash the bare serial-only recovery firmware.
- [scripts/serial-reset-log.sh](scripts/serial-reset-log.sh): capture boot logs with direct DTR/RTS reset wiring.
- [scripts/watch-usb.sh](scripts/watch-usb.sh): watch kernel USB/TTY events while replugging adapters.

## Main Board Facts

- ESP32 target: ESP32-U4WD, configured as ESPHome `esp32dev` with ESP-IDF.
- Flash mode: DIO. The bootloader reports `mode:DIO`; this is important because the CAN pins are GPIO9/GPIO10, which ESPHome warns about for QIO flash configurations.
- CAN: `GPIO10` TX and `GPIO9` RX through the TPT7721 isolator.
- CAN side power: requires the board's normal 12 V input. USB-only ESP32 power is not sufficient for CAN validation.
- Status LEDs are active-low: LINK `GPIO2`, ERR `GPIO4`, CAN `GPIO23`.
- Ethernet PHY: RTL8201-compatible RMII PHY with `phy_addr: 0`.

## Confirmed RTL8201 Wiring

| RTL8201 signal | ESP32 GPIO |
| --- | --- |
| PHYRSTB | GPIO14 |
| MDC | GPIO18 |
| MDIO | GPIO5 |
| TXEN | GPIO21 |
| RXD0 | GPIO25 |
| RXD1 | GPIO26 |
| TXD0 | GPIO19 |
| TXD1 | GPIO22 |
| CRS_DV | GPIO27 |
| TXC / REF_CLK | GPIO0 |

The MDIO scanner found a live PHY at addresses 0 and 1 with `phy_id=0x001C:0xC816`. The production firmware intentionally uses `phy_addr: 0`.

## Useful Development Commands

Run commands from the repository root.

Bare serial-only firmware:

```sh
./scripts/esphome.sh config dev/configs/ecan-e02-bare.yaml
./scripts/esphome.sh compile dev/configs/ecan-e02-bare.yaml
./dev/scripts/flash-bare.sh /dev/ttyACM0
```

CAN self-test:

```sh
./scripts/esphome.sh compile dev/configs/ecan-e02-can-self-test.yaml
./scripts/esphome.sh upload dev/configs/ecan-e02-can-self-test.yaml --device /dev/ttyACM0
```

RTL8201 MDIO scan:

```sh
./scripts/esphome.sh compile dev/configs/ecan-e02-ethernet-mdio-scan.yaml
./scripts/esphome.sh upload dev/configs/ecan-e02-ethernet-mdio-scan.yaml --device /dev/ttyACM0
```

Boot-log capture with direct DTR/RTS wiring:

```sh
./dev/scripts/serial-reset-log.sh /dev/ttyACM0 60
```

## Validation History

The following were validated with ESPHome 2026.5.x, most recently 2026.5.3:

- [../configs/ecan-e02.yaml](../configs/ecan-e02.yaml): config, compile, flash, and stable boot logs; CAN initializes in `LISTENONLY` mode, and Ethernet starts with RTL8201 `phy_addr: 0`.
- [configs/ecan-e02-bare.yaml](configs/ecan-e02-bare.yaml): config and compile.
- [configs/ecan-e02-led-test.yaml](configs/ecan-e02-led-test.yaml): config and compile.
- [configs/ecan-e02-can-listen.yaml](configs/ecan-e02-can-listen.yaml): config.
- [configs/ecan-e02-can-self-test.yaml](configs/ecan-e02-can-self-test.yaml): config, compile, flash, and repeated `CAN self-test PASS` logs when powered from the board's intended 12 V input.
- [configs/ecan-e02-can-listen.yaml.example](configs/ecan-e02-can-listen.yaml.example): config and compile.
- [configs/ecan-e02-ethernet-rtl8201.yaml.example](configs/ecan-e02-ethernet-rtl8201.yaml.example): config and compile.
- [configs/ecan-e02-gpio-probe.yaml.example](configs/ecan-e02-gpio-probe.yaml.example): config.
- [configs/ecan-e02-ethernet-mdio-scan.yaml](configs/ecan-e02-ethernet-mdio-scan.yaml): config, compile, flash, and PHY ID logs on confirmed `GPIO18` MDC, `GPIO5` MDIO, and `GPIO14` reset.

Ethernet link and DHCP still need to be verified with the RJ45 port connected to a live network.
