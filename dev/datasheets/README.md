# Ebyte ECAN-E02 Datasheets

This directory contains local convenience copies of datasheets for confirmed Ebyte ECAN-E02 ICs, likely board ICs, and bench hardware used during bring-up. The firmware build does not depend on these files; they are kept here so hardware tracing and future board-variant checks do not depend on finding the same PDFs again.

Prefer official manufacturer PDFs where they are directly available. A few parts use distributor or mirror-hosted PDFs because the manufacturer page is gated, uses a JavaScript download shell, or does not expose a stable direct PDF URL.

## Files

| File | Part | Project role | Source | Notes |
| --- | --- | --- | --- | --- |
| [ESP32-Series-Datasheet.pdf](ESP32-Series-Datasheet.pdf) | Espressif ESP32 series, including ESP32-U4WD/U4WDH package details | Main MCU target | <https://documentation.espressif.com/esp32_datasheet_en.pdf> | Used for package pins, flash pin caveats, and GPIO capability checks. |
| [RTL8201F-VB-CG-Datasheet.pdf](RTL8201F-VB-CG-Datasheet.pdf) | Realtek RTL8201F | RMII Ethernet PHY | <https://datasheet4u.com/pdf/845737/RTL8201F-VB-CG.pdf> | Mirror-hosted copy; used for RMII pins, PHY address straps, LED pins, reset, and REF_CLK behavior. |
| [TPT772x-Datasheet.pdf](TPT772x-Datasheet.pdf) | 3PEAK TPT7721/TPT772x | Dual-channel digital isolator between ESP32 TWAI and CAN-side transceiver | <https://static.3peak.com/res/doc/ds/Datasheet_TPT772x.pdf> | Confirmed CAN path uses ESP32-side `IN1` and `OUT2`. |
| [SIT65HVD233-Datasheet.pdf](SIT65HVD233-Datasheet.pdf) | SIT65HVD233 | 3.3 V CAN transceiver family matching the observed `SIT65MV0233` marking | <https://www.micros.com.pl/mediaserver/UISN65hvd233d_SIT_0001.pdf> | Used for CANH/CANL, TXD/RXD, RS, LBK, and supply pin checks. |
| [MIC5233-Datasheet.pdf](MIC5233-Datasheet.pdf) | Microchip/Micrel MIC5233 | CAN-side LDO feeding the transceiver-side logic rail | <https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MIC5233-Data-Sheet-DS20006033.pdf> | Confirmed 5-pin `M 5233` package; input and enable are tied high on the board. |
| [AMS1117-3.3-Datasheet.pdf](AMS1117-3.3-Datasheet.pdf) | AMS1117-3.3 | 3.3 V linear regulator | <https://www.datasheets.com/Advanced-Monolithic-Systems/AMS1117-3.3/datasheet.pdf> | SOT-223 part has no enable pin. |
| [MORNSUN-B_S-1WR3-Datasheet.pdf](MORNSUN-B_S-1WR3-Datasheet.pdf) | MORNSUN B0505S-1WR3 | 5 V to isolated 5 V DC/DC module for the CAN-side power domain | <https://www.mornsun-power.com/public/uploads/pdf/B_S-1WR3.pdf> | Confirmed large rectangular `B0505S-1WR3` module. |
| [ESDCAN24-2BLY-Datasheet.pdf](ESDCAN24-2BLY-Datasheet.pdf) | ST ESDCANxx-2BLY family, including ESDCAN24-2BLY | CANH/CANL ESD/TVS protection device | <https://www.hkjdwchip.com/image/files/stmicroelectronics-esdcan062bly-datasheets-0595.pdf> | ST family PDF explicitly lists ESDCAN24-2BLY; official ST URL is <https://www.st.com/resource/en/datasheet/esdcan24-2bly.pdf>. |
| [CH343DS1-Datasheet.pdf](CH343DS1-Datasheet.pdf) | WCH CH343 | External USB-UART adapter used for flashing and logs | <https://cdn-learn.adafruit.com/assets/assets/000/134/549/original/CH343DS1.PDF?1737477957> | Local copy is from Adafruit's CDN; WCH's public download page is <https://www.wch-ic.com/downloads/CH343DS1_PDF.html>. |
| [SN65HVD23x-Datasheet.pdf](SN65HVD23x-Datasheet.pdf) | TI SN65HVD230/SN65HVD23x | Reference for the VP230/SN65HVD230 bench CAN peer | <https://www.ti.com/lit/ds/symlink/sn65hvd230.pdf> | Used for the external ESP32-S3-Zero CAN peer test setup. |
| [RY8310-Datasheet.candidate.pdf](RY8310-Datasheet.candidate.pdf) | RYCHIP RY8310 | Candidate comparison datasheet for the unconfirmed `RDA2L 5T4V.1` SOT23-6 power IC | <https://semic-boutique.com/wp-content/uploads/2021/01/RY8310-Datasheet-V1.0.0.pdf> | Candidate only. The RY8310 datasheet top marking is `GBYLL`, so the board's `RDA2L` marking still needs confirmation before treating this as the actual fitted part. |

## Confidence Notes

- Confirmed board parts: ESP32-U4WD target, RTL8201-compatible PHY, TPT7721 isolator, SIT65HVD233-style CAN transceiver, MIC5233, AMS1117-3.3, B0505S-1WR3, and ESDCAN24-2BLY-style CAN protection.
- External bench hardware: CH343 USB-UART adapter and SN65HVD230/VP230 CAN peer transceiver.
- Candidate-only part: RY8310. Keep it useful for pin/function comparison, but do not update production documentation as a confirmed part until the `RDA2L 5T4V.1` package is traced or otherwise identified.
