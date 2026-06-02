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

## Status LEDs

Observed ECAN-E02 status LED traces:

| LED label | ESP32 package pin | ESP32 GPIO | Notes |
| --- | --- | --- | --- |
| LINK | 22 | GPIO2 | GPIO2 is a boot strap pin; only drive it after boot. |
| ERR | 24 | GPIO4 | Use `configs/ecan-e02-led-test.yaml` for polarity testing. |
| CAN | 36 | GPIO23 | Conflicts with the common ESP32 RMII `MDC` default, so trace Ethernet management pins before enabling RTL8201. |

The LED test firmware drives LINK, ERR, then CAN for 500 ms each. If the LEDs are active-low, set `status_led_inverted: "true"` in `configs/ecan-e02-led-test.yaml`.

## CAN transceiver

Find the CAN transceiver package first. Common markings are SN65HVD230, TJA1050, TJA1051, MCP2562, VP230, or similar.

Observed ECAN-E02 CAN path:

```text
ESP32 package pin 28 / GPIO10 -> TPT7721 IN1
ESP32 package pin 27 / GPIO9  <- TPT7721 OUT2
TPT7721      <-> SIT65HVD233-style CAN transceiver
```

On the TPT7721 SOP8 top-view pinout, this means the ESP32 side is using `pin 7 = IN1` for CAN TX and `pin 6 = OUT2` for CAN RX. The transceiver side of those isolated channels is confirmed as `pin 2 = OUT1` driving SIT `pin 1 = D/TXD`, and SIT `pin 4 = R/RXD` driving TPT7721 `pin 3 = IN2`.

The SIT65HVD233-style transceiver `LBK` pin is tied to ground, so hardware transceiver loopback is disabled. Without another CAN node, use `configs/ecan-e02-can-self-test.yaml` for an ESP32 TWAI no-ACK self-reception test. That verifies the ESP32 TWAI peripheral and GPIO routing, but it does not prove the CANH/CANL physical bus path.

Current CAN self-test status: with a 120 ohm resistor across CANH/CANL, TWAI starts successfully on `GPIO10` TX and `GPIO9` RX at `500KBPS`, then the first self-test frame drives the controller to `BUS_OFF` with `tx_err=128`, `tx_fail=1`, and `bus_err=16`. That points to the transmitted bitstream not being observed correctly on RX through the isolator/transceiver path, or the transceiver being disabled/not powered.

If the SIT `RS` pin is too difficult to probe directly, use `configs/ecan-e02-can-gpio-probe.yaml`. It pulses ESP32 `GPIO10` low briefly and samples ESP32 `GPIO9` without starting TWAI, so the controller cannot enter bus-off while the physical TX/RX path is checked. Current GPIO-probe status: direct `gpio_set_level(GPIO10, 1)` still reads back `tx_gpio=0`, while `GPIO9` stays high. That points to the TX line being held low, GPIO10 not being usable as assumed, or the package/GPIO mapping needing re-check.

Observed CAN protection: board designator `D2` is a 3-pin SOT-23 part marked `EL24`, which matches ST `ESDCAN24-2BLY`, a dual-line CAN TVS protector. This device should be on CANH/CANL and CAN-side ground. If an apparent SIT `RS` trace reaches `D2`, recheck the SIT pin orientation because adjacent SIT pins 6 and 7 are the expected CANL/CANH pins.

Observed local power IC: a 5-pin `M 5233` package is confirmed as Microchip/Micrel MIC5233. Its input and enable are tied high, and its output supplies SIT `VCC` plus TPT7721 `VCCA` on the CAN/transceiver side.

| MIC5233 pin | Signal | Status |
| --- | --- | --- |
| 1 | IN | Tied high. |
| 2 | GND | Regulator ground for the CAN/transceiver-side rail. |
| 3 | EN | Tied high. |
| 4 | NC/ADJ | Fixed-output parts leave this unconnected; adjustable parts use a divider. |
| 5 | OUT | Supplies SIT `VCC` and TPT7721 `VCCA`. |

Other observed power devices:

| Marking | Likely role | Notes |
| --- | --- | --- |
| `RDA2L 5T4V.1` | Likely RY8310-class SOT23-6 buck regulator if near an inductor and feedback resistors | Candidate only until the package pins are traced. RY8310 devices include an `EN` pin and generate an adjustable local rail from a higher input. |
| `AMS1117-3.3` | Fixed 3.3 V linear regulator | No enable pin. In SOT-223, pin 1 is ground, pin 2/tab is output, and pin 3 is input. It is on whenever its input rail is present. |

The matching ESPHome listen-only CAN pins are:

```yaml
tx_pin: GPIO10
rx_pin: GPIO9
```

GPIO9 and GPIO10 are `SD_DATA_2` and `SD_DATA_3` package pins in the ESP32 pin table. On the ESP32-U4WD target, the in-package flash mapping does not use these two pins, but keep the first CAN firmware in `LISTENONLY` mode until bus RX is confirmed.

Do not short CANH directly to CANL for testing. If the bus side needs a local load, use approximately 120 ohms across CANH/CANL.

Trace these transceiver pins:

| Transceiver signal | Trace to | Notes |
| --- | --- | --- |
| TXD | ESP32 package pin 28 / GPIO10 through TPT7721 | Confirmed path: ESP32 `GPIO10` -> TPT7721 `IN1` -> TPT7721 `OUT1` -> SIT `D/TXD`. |
| RXD | ESP32 package pin 27 / GPIO9 through TPT7721 | Confirmed path: SIT `R/RXD` -> TPT7721 `IN2` -> TPT7721 `OUT2` -> ESP32 `GPIO9`. |
| RS | Resistor, destination/value not yet confirmed | If this is an HVD233-compatible transceiver, a resistor to ground selects slope-control mode; high level selects standby. Measure powered `RS` voltage and trace the resistor's other end. |
| CANH/CANL | terminal/connector and D2/EL24 TVS | Confirm connector orientation and SIT pin orientation. |
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
| MDC | Trace required | ESPHome requires `mdc_pin`; GPIO23 is already traced to the CAN LED. |
| MDIO | GPIO18 | ESPHome requires `mdio_pin`. |
| REF_CLK | GPIO0 input, GPIO16 output, or GPIO17 output | ESPHome requires `clk.mode` and `clk.pin`. |
| PHYAD straps | address 0 or 1 are common | ESPHome requires `phy_addr`. |
| RESET or POWER_EN | sometimes GPIO16/17 or tied high | ESPHome may need `power_pin`. |

Start with the example in `configs/ecan-e02-ethernet-rtl8201.yaml.example` only after replacing MDC, MDIO, REF_CLK, and PHY address with confirmed traces.

GPIO0 is also a boot strap pin. If the board uses GPIO0 as RMII REF_CLK input, the PHY clock circuit must not prevent normal boot mode.

## Passive GPIO firmware probing

After the bare firmware works, copy `configs/ecan-e02-gpio-probe.yaml.example` to a real YAML file and edit the `probe_pins` list. The `ecan_e02` component will configure those pins as inputs and log their levels in order.

Do not probe:

- GPIO6-GPIO11, used by ESP32 flash
- pins that are already active in another component
- pins connected to external 5 V logic unless the trace confirms level shifting
