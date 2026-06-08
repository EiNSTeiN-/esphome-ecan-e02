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

The LED test firmware drives LINK, ERR, then CAN for 500 ms each. The LEDs appear to be active-low: with `status_led_inverted: "false"`, the earlier test made the selected LED blink off rather than on. `configs/ecan-e02-led-test.yaml` therefore uses `status_led_inverted: "true"` so each log label should match a visible LED-on pulse.

The main firmware in `configs/ecan-e02.yaml` uses the same active-low polarity. LINK follows Ethernet connect/disconnect, ERR is ESPHome's status LED, and CAN pulses on CAN RX or test TX activity.

## CAN transceiver

Find the CAN transceiver package first. Common markings are SN65HVD230, TJA1050, TJA1051, MCP2562, VP230, or similar.

Observed ECAN-E02 CAN path:

```text
ESP32 package pin 29 / GPIO10 -> TPT7721 IN1
ESP32 package pin 28 / GPIO9  <- TPT7721 OUT2
TPT7721      <-> SIT65HVD233-style CAN transceiver
```

On the TPT7721 SOP8 top-view pinout, this means the ESP32 side is using `pin 7 = IN1` for CAN TX and `pin 6 = OUT2` for CAN RX. The transceiver side of those isolated channels is confirmed as `pin 2 = OUT1` driving SIT `pin 1 = D/TXD`, and SIT `pin 4 = R/RXD` driving TPT7721 `pin 3 = IN2`.

Unpowered continuity checks confirm TPT7721 `pin 7 = IN1` connects only to ESP32 package pin 29 / `GPIO10`, with no continuity to other TPT7721 pins. The measured resistance from TPT7721 pin 7 to ESP-side ground is about 1.15 Mohm, and to ESP-side 3.3 V is about 1.20 Mohm. TPT7721 `pin 6 = OUT2` connects to ESP32 package pin 28 / `GPIO9` and is not shorted to adjacent pins. These measurements rule out a static low-ohm short on the ESP32-side TX/RX paths.

The SIT65HVD233-style transceiver `LBK` pin is tied to ground, so hardware transceiver loopback is disabled. Without another CAN node, use `configs/ecan-e02-can-self-test.yaml` for an ESP32 TWAI no-ACK self-reception test. That verifies the ESP32 TWAI peripheral and GPIO routing, but it does not prove the CANH/CANL physical bus path.

Current CAN self-test status: with the ECAN-E02 powered from its intended 12 V supply input and a 120 ohm resistor across CANH/CANL, `configs/ecan-e02-can-self-test.yaml` passes repeatedly on `GPIO10` TX and `GPIO9` RX at `500KBPS`. The self-test firmware logs `CAN self-test PASS` frames with `id=0x321`.

Do not treat ESP32-side 3.3 V test-pad power as sufficient for CAN validation. The CAN/transceiver side is powered through the board power path, including the isolated DC/DC module and MIC5233 rail. If only the ESP32 side is powered, the CAN side may be unpowered and TWAI/CAN results can look like a broken RX return path.

If the SIT `RS` pin is too difficult to probe directly, use `configs/ecan-e02-can-gpio-probe.yaml`. It pulses ESP32 `GPIO10` low briefly and samples ESP32 `GPIO9` without starting TWAI, so the controller cannot enter bus-off while the physical TX/RX path is checked. Current GPIO-probe status: after switching `GPIO10` to input/output mode, `GPIO10` readback is fixed and follows the commanded level (`tx_gpio=1` high, `tx_gpio=0` low). `GPIO9` still remains high during the low TX pulse, so the remaining issue is beyond ESP32 `GPIO10` output readback: check the TPT7721 output side, CAN transceiver enable/power, and RX return path.

For meter-based powered probing, flash `configs/ecan-e02-can-meter-probe.yaml`. It holds `GPIO10` high for 5 seconds, then low for 5 seconds. Measure each point relative to its local ground:

| Probe point | Local ground | Expected when `TXD HIGH` | Expected when `TXD LOW` |
| --- | --- | --- | --- |
| TPT7721 pin 7 `IN1` | TPT7721 pin 5 `GNDB` | ESP-side 3.3 V | 0 V |
| TPT7721 pin 2 `OUT1` | TPT7721 pin 4 `GNDA` | CAN-side logic high | CAN-side logic low |
| SIT pin 1 `D/TXD` | SIT pin 2 `GND` | CAN-side logic high | CAN-side logic low |
| SIT pin 4 `R/RXD` | SIT pin 2 `GND` | CAN-side logic high | Should go low if the transceiver and terminated bus are responding to dominant TX |
| TPT7721 pin 3 `IN2` | TPT7721 pin 4 `GNDA` | Same as SIT `R/RXD` | Same as SIT `R/RXD` |
| TPT7721 pin 6 `OUT2` | TPT7721 pin 5 `GNDB` | ESP-side logic high | Should go low if the RX return channel is working |

If TPT7721 pin 7 toggles but pin 2 does not, suspect TPT7721 CAN-side power or the isolator. If pin 2 and SIT pin 1 toggle but SIT pin 4 does not, focus on SIT power, CANH/CANL, and termination. If SIT pin 4 toggles but TPT7721 pin 6 does not, focus on the RX isolator channel.

SIT pin 8 `RS` has about 180 mohm continuity to SIT pin 2 `GND`, so the transceiver should be in normal/high-speed mode rather than standby through `RS`.

Observed CAN protection: board designator `D2` is a 3-pin SOT-23 part marked `EL24`, which matches ST `ESDCAN24-2BLY`, a dual-line CAN TVS protector. This device should be on CANH/CANL and CAN-side ground. If an apparent SIT `RS` trace reaches `D2`, recheck the SIT pin orientation because adjacent SIT pins 6 and 7 are the expected CANL/CANH pins.

Observed local power IC: a 5-pin `M 5233` package is confirmed as Microchip/Micrel MIC5233. Its input and enable are tied high, and its output supplies SIT `VCC` plus TPT7721 `VCCA` on the CAN/transceiver side.

Observed TPT7721 supply pins: TPT7721 `pin 8 = VCCB` has about 200 mohm continuity to ESP-side 3.3 V, and `pin 5 = GNDB` has about 200 mohm continuity to ESP-side ground. TPT7721 `pin 1 = VCCA` connects to MIC5233 output, and `pin 4 = GNDA` connects to MIC5233 ground. TPT7721 `GNDA` and `GNDB` measure about 500 ohm apart on the assembled board, so the two ground domains are not hard-shorted but do have a measurable DC path that should be traced.

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
| `B0505S-1WR3` | 1 W isolated 5 V to 5 V DC/DC module | Likely generates the isolated CAN-side 5 V rail. Its output should feed the CAN-side regulator/power path, including the MIC5233 input rail. |
| `RDA2L 5T4V.1` | Likely RY8310-class SOT23-6 buck regulator if near an inductor and feedback resistors | Candidate only until the package pins are traced. RY8310 devices include an `EN` pin and generate an adjustable local rail from a higher input. |
| `AMS1117-3.3` | Fixed 3.3 V linear regulator | No enable pin. In SOT-223, pin 1 is ground, pin 2/tab is output, and pin 3 is input. It is on whenever its input rail is present. |

The matching ESPHome listen-only CAN pins are:

```yaml
tx_pin: GPIO10
rx_pin: GPIO9
```

GPIO9 and GPIO10 are `SD_DATA_2` and `SD_DATA_3` package pins in the ESP32 pin table. ESPHome warns that these pins may be used by flash in quad-I/O mode, but this project explicitly sets `board_build.flash_mode: dio`; generated ESP-IDF config and boot logs both report `DIO`. Espressif's ESP32-U4WDH in-package flash mapping uses `GPIO16`, `GPIO17`, `SD_DATA_0`, `SD_DATA_1`, `SD_CMD`, and `SD_CLK`, not `SD_DATA_2` or `SD_DATA_3`. That makes flash-mode clobbering unlikely for this board. Keep the first CAN firmware in `LISTENONLY` mode until bus RX is confirmed.

Do not short CANH directly to CANL for testing. If the bus side needs a local load, use approximately 120 ohms across CANH/CANL.

Trace these transceiver pins:

| Transceiver signal | Trace to | Notes |
| --- | --- | --- |
| TXD | ESP32 package pin 29 / GPIO10 through TPT7721 | Confirmed path: ESP32 `GPIO10` -> TPT7721 `IN1` -> TPT7721 `OUT1` -> SIT `D/TXD`. |
| RXD | ESP32 package pin 28 / GPIO9 through TPT7721 | Confirmed path: SIT `R/RXD` -> TPT7721 `IN2` -> TPT7721 `OUT2` -> ESP32 `GPIO9`. |
| RS | SIT pin 2 `GND` | About 180 mohm to ground. For an HVD233-compatible transceiver, this should select normal/high-speed mode rather than standby. |
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

Observed ECAN-E02 RTL8201 traces:

| RTL8201 signal | RTL8201 pin | ESP32 package pin | ESP32 GPIO | Status |
| --- | --- | --- | --- | --- |
| PHYRSTB | 21 | 17 | GPIO14 | Confirmed. Active-low reset; MDIO scanner pulses this before reading. |
| MDC | 22 | 35 | GPIO18 | Confirmed. |
| MDIO | 23 | 34 | GPIO5 | Confirmed. GPIO5 is a boot strap pin; the board boots normally with this trace. |
| TXEN | 20 | 42 | GPIO21 | Confirmed fixed RMII signal. |
| RXD0 | - | 14 | GPIO25 | Confirmed fixed RMII signal. |
| RXD1 | - | 15 | GPIO26 | Confirmed fixed RMII signal. |
| TXD0 | - | 38 | GPIO19 | Confirmed fixed RMII signal. |
| TXD1 | - | 39 | GPIO22 | Confirmed fixed RMII signal. |
| CRS_DV | 26 | 16 | GPIO27 | Confirmed fixed RMII signal. |
| TXC / REF_CLK | 15 | 23 | GPIO0 | Confirmed. PHY clock output feeds ESP32 `CLK_EXT_IN`. |
| RXD3 / CLK_CTL | 12 | Unpopulated pads | Not connected to ESP32 | Floating/unpopulated strap should use the RTL8201F internal pulldown, selecting REF_CLK output mode. |
| LED0 / PHYAD0 / PMEB | 24 | Trace next | Trace next | PHY address bit 0 and LED strap on RTL8201F QFN-32. |
| LED1 / PHYAD1 | 25 | Trace next | Trace next | PHY address bit 1 and LED strap on RTL8201F QFN-32. |

The MDIO scanner confirms a live RTL8201-compatible PHY on the traced management pins:

```text
MDC GPIO18, MDIO GPIO5, reset GPIO14
MDIO PHY addr=0 bmcr=0x1000 bmsr=0x7849 phy_id=0x001C:0xC816
MDIO PHY addr=1 bmcr=0x1000 bmsr=0x7849 phy_id=0x001C:0xC816
```

Use `phy_addr: 0` for the first ESPHome Ethernet attempt. Address 1 currently reads as the same PHY as address 0, so treat it as an alias or strap behavior until the PHY address pins are traced.

The REF_CLK path now matches the checked-in ESPHome Ethernet skeleton: RTL8201F pin 15 drives ESP32 `GPIO0` in `CLK_EXT_IN` mode. RTL8201F pin 12 appears to go only to unpopulated pads; if left floating, the chip's internal pulldown selects REF_CLK output mode.

GPIO0 is also a boot strap pin. If the board uses GPIO0 as RMII REF_CLK input, the PHY clock circuit must not prevent normal boot mode.

Use `configs/ecan-e02-ethernet-mdio-scan.yaml` to verify traced MDC/MDIO candidates before enabling full Ethernet. The scanner bit-bangs IEEE 802.3 Clause 22 management reads and logs PHY ID candidates across addresses 0 through 31. It does not enable the ESP32 Ethernet MAC or RMII data pins.

The checked-in scan substitutions use the confirmed ECAN-E02 management pins:

```yaml
mdc_pin: GPIO18
mdio_pin: GPIO5
reset_pin: GPIO14
```

The scanner rejects GPIO6-GPIO11 and GPIO16/GPIO17. If the scan is correct and the PHY is powered/out of reset, logs should contain a line like:

```text
MDIO PHY addr=0 bmcr=0x.... bmsr=0x.... phy_id=0x....:0x....
```

If it logs `MDIO scan found no plausible PHY`, recheck PHY power/reset, MDC/MDIO continuity, and PHY address straps.

For an RTL8201F QFN-32, useful remaining physical trace points are:

| RTL8201F pin | Signal | Trace target |
| --- | --- | --- |
| 24 | LED0 / PHYAD0 / PMEB | Confirm pull direction and whether it is routed through an LED/resistor network. |
| 25 | LED1 / PHYAD1 | Confirm pull direction and whether it is routed through an LED/resistor network. |

If the package is RTL8201FL/FN 48-pin instead, use the datasheet pin table rather than the QFN-32 pin numbers above.

## Passive GPIO firmware probing

After the bare firmware works, copy `configs/ecan-e02-gpio-probe.yaml.example` to a real YAML file and edit the `probe_pins` list. The `ecan_e02` component will configure those pins as inputs and log their levels in order.

Do not probe:

- GPIO6-GPIO11, used by ESP32 flash
- GPIO16/GPIO17, used by ESP32-U4WD in-package flash
- pins that are already active in another component
- pins connected to external 5 V logic unless the trace confirms level shifting
