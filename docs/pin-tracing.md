# ECAN-E02 pin tracing checklist

Use this while the board is unpowered unless a step explicitly says to boot the firmware. Continuity mode is usually enough for the first pass.

## First flash

1. Plug in the CH343 USB cable.
2. Run `./scripts/serial-port.sh`.
3. Run `./scripts/flash-bare.sh <port>` if auto-detect does not pick the right port.
4. Run `./scripts/logs.sh <port>` and confirm the `ecan_e02` boot diagnostics appear.

If upload fails to enter bootloader, hold BOOT, tap RESET, start upload, then release BOOT when esptool starts connecting.

For repeatable automated flashing, wire the USB-UART control lines to the ESP32:

| USB-UART signal | ESP32 signal |
| --- | --- |
| `DTR` | `GPIO0` / `BOOT` |
| `RTS` | `EN` / `CHIP_PU` / `RESET` |

If those lines are directly wired during bench bring-up, use `./scripts/serial-reset-log.sh <port>` for boot log capture. It keeps `DTR` and `RTS` inactive while reading logs and pulses `RTS` only for reset.

## CAN transceiver

Find the CAN transceiver package first. Common markings are SN65HVD230, TJA1050, TJA1051, MCP2562, VP230, or similar.

Observed ECAN-E02 CAN path:

```text
ESP32 GPIO10 -> TPT7721 IN2
ESP32 GPIO9  <- TPT7721 OUT2
TPT7721      <-> SIT65HVD233-style CAN transceiver
```

The matching ESPHome listen-only CAN pins are:

```yaml
tx_pin: GPIO10
rx_pin: GPIO9
```

GPIO9 and GPIO10 are `SD_DATA_2` and `SD_DATA_3` package pins in the ESP32 pin table. On the ESP32-U4WD target, the in-package flash mapping does not use these two pins, but keep the first CAN firmware in `LISTENONLY` mode until bus RX is confirmed.

Trace these transceiver pins:

| Transceiver signal | Trace to | Notes |
| --- | --- | --- |
| TXD | ESP32 GPIO | This becomes ESPHome `tx_pin`. |
| RXD | ESP32 GPIO | This becomes ESPHome `rx_pin`. |
| STB, S, EN, RS | ESP32 GPIO or rail | If tied to a GPIO, we may need to drive it before CAN works. |
| CANH/CANL | terminal/connector | Confirm connector orientation. |
| VCC/VIO | 3.3 V or 5 V rail | Determines whether the logic side is ESP32-safe. |
| GND | board ground | Confirm common ground. |

The ESP32 TWAI peripheral can route TX/RX through the GPIO matrix, so the TXD/RXD pins are not fixed. Once TXD/RXD are known, copy `configs/ecan-e02-can-listen.yaml.example`, set `can_tx_pin` and `can_rx_pin`, and start in `LISTENONLY` mode.

Only switch `can_mode` to `NORMAL` after the bit rate and transceiver enable/standby pin are understood.

## RTL8201 Ethernet PHY

Classic ESP32 RMII has fixed data pins:

| ESP32 GPIO | RMII signal |
| --- | --- |
| GPIO19 | TXD0 |
| GPIO21 | TX_EN |
| GPIO22 | TXD1 |
| GPIO25 | RXD0 |
| GPIO26 | RXD1 |
| GPIO27 | CRS_DV |

On a typical RTL8201 design, these are the variable or board-specific signals to confirm:

| Signal | Common ESP32 GPIO | Why it matters |
| --- | --- | --- |
| MDC | GPIO23 | ESPHome requires `mdc_pin`. |
| MDIO | GPIO18 | ESPHome requires `mdio_pin`. |
| REF_CLK | GPIO0 input, GPIO16 output, or GPIO17 output | ESPHome requires `clk.mode` and `clk.pin`. |
| PHYAD straps | address 0 or 1 are common | ESPHome requires `phy_addr`. |
| RESET or POWER_EN | sometimes GPIO16/17 or tied high | ESPHome may need `power_pin`. |

Start with the example in `configs/ecan-e02-ethernet-rtl8201.yaml.example` only if traces match MDC GPIO23, MDIO GPIO18, REF_CLK on GPIO0, and PHY address 0.

GPIO0 is also a boot strap pin. If the board uses GPIO0 as RMII REF_CLK input, the PHY clock circuit must not prevent normal boot mode.

## Passive GPIO firmware probing

After the bare firmware works, copy `configs/ecan-e02-gpio-probe.yaml.example` to a real YAML file and edit the `probe_pins` list. The `ecan_e02` component will configure those pins as inputs and log their levels in order.

Do not probe:

- GPIO6-GPIO11, used by ESP32 flash
- pins that are already active in another component
- pins connected to external 5 V logic unless the trace confirms level shifting
