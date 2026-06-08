# External CAN peer

Use a bare `VP230`/`SN65HVD230`-style 3.3 V CAN transceiver with a Waveshare ESP32-S3-Zero as an independent CAN peer. This lets the ECAN-E02 CANH/CANL path be tested without relying only on the ECAN-E02's internal RX return path.

## VP230 wiring

Use `dev/configs/can-peer-esp32-s3-zero-vp230.yaml` for the ESP32-S3-Zero.

| VP230 pin | Signal | Connect to |
| --- | --- | --- |
| 1 | `D` / `TXD` | ESP32-S3-Zero `RX` silkscreen / `GPIO44`, configured as CAN TX |
| 2 | `GND` | ESP32-S3-Zero `GND`; also connect to ECAN-E02 CAN-side ground if available |
| 3 | `VCC` | ESP32-S3-Zero `3V3` |
| 4 | `R` / `RXD` | ESP32-S3-Zero `TX` silkscreen / `GPIO43`, configured as CAN RX |
| 5 | `VREF` | Leave open |
| 6 | `CANL` | ECAN-E02 `CANL` |
| 7 | `CANH` | ECAN-E02 `CANH` |
| 8 | `RS` | ESP32-S3-Zero `GND` |

Do not power the VP230 from 5 V. It is a 3.3 V CAN transceiver.

The ESP32-S3-Zero `TX`/`RX` pads are only being used as GPIOs here. The test firmware logs over native USB serial/JTAG, so the default UART0 pins are available for the CAN controller. Do not wire this as UART traffic; the VP230 logic pins are CAN controller TX/RX signals.

If a VP230 breakout labels these pins from the controller's point of view instead of the transceiver's point of view, trust the IC pin function over the silkscreen: VP230/SN65HVD230 pin 1 `D`/`TXD` goes to ESP CAN TX, and pin 4 `R`/`RXD` goes to ESP CAN RX.

Make sure the ECAN-E02 is powered in the same way it will be during the test. The isolated CAN-side regulator must be on, otherwise the external peer can be wired correctly and still see no valid peer on CANH/CANL.

For a short bench bus with only the ECAN-E02 and this peer, use 120 ohm termination across CANH/CANL at each end if possible. With two 120 ohm terminators installed, CANH-to-CANL measures about 60 ohm unpowered. With one terminator installed, it measures about 120 ohm; this may still work over short leads but is less ideal at 500 kbit/s.

## Test firmware

Flash the peer:

```sh
./scripts/esphome.sh upload dev/configs/can-peer-esp32-s3-zero-vp230.yaml --device /dev/ttyACM0
```

The ESP32-S3-Zero has native USB. If it does not appear as a serial device, hold `BOOT` while plugging it into USB to enter download mode.

Flash the ECAN-E02 normal transmit test:

```sh
./scripts/esphome.sh upload dev/configs/ecan-e02-can-normal-tx.yaml --device /dev/ttyACM0
```

The ECAN-E02 firmware sends `can_id=0x602` every 2 seconds. The ESP32-S3-Zero peer sends `can_id=0x711` every 5 seconds and logs any received frames.

## Expected outcomes

If the ESP32-S3-Zero peer logs `id=0x00000602`, the ECAN-E02 physical TX path is working through ESP32 `GPIO10`, TPT7721, SIT transceiver, and CANH/CANL.

If the ECAN-E02 logs `id=0x00000711`, the ECAN-E02 physical RX path is working through CANH/CANL, SIT transceiver, TPT7721, and ESP32 `GPIO9`.

If the peer logs `0x602` but ECAN-E02 does not log `0x711`, the ECAN-E02 CAN output path works but the RX return path is still suspect.

If neither side logs received frames, focus on VP230 wiring, CANH/CANL orientation, shared CAN-side ground, termination, SIT power, and the TPT7721/SIT TX path.

## References

- Waveshare ESP32-S3-Zero documentation: <https://docs.waveshare.com/ESP32-S3-Zero/>
- TI SN65HVD230 datasheet, also used by Waveshare for VP230 modules: <https://files.waveshare.com/upload/8/82/SN65HVD230.pdf>
