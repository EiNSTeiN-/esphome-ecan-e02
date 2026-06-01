# Repository Guidelines

## Project Structure

This is an ESPHome bring-up workspace for an ECAN-E02 board with an ESP32-U4WD target, CH343 USB serial, likely RTL8201 RMII Ethernet, and an onboard CAN transceiver.

- `components/ecan_e02/` contains the local ESPHome external component used for board diagnostics and passive GPIO probing.
- `configs/` contains ESPHome YAMLs. `ecan-e02-bare.yaml` is the safe first-flash target.
- `scripts/` contains local helper commands that run ESPHome through `uvx`.
- `docs/` contains hardware tracing notes. Update these as pins are confirmed.

## Build And Flash Commands

Use the helper script so ESPHome can run from `uvx` without installing globally:

```sh
./scripts/esphome.sh config configs/ecan-e02-bare.yaml
./scripts/esphome.sh compile configs/ecan-e02-bare.yaml
./scripts/flash-bare.sh
./scripts/logs.sh
```

The scripts use `/tmp/uv-cache` and `/tmp/uv-tools` by default to avoid writing tool state into the home directory.

`make config`, `make compile`, `make flash`, and `make logs` wrap the same commands.

## Coding Style

Follow ESPHome component conventions:

- Python schema files define `CONFIG_SCHEMA` and `async def to_code(config)`.
- C++ components live beside the schema and use ESPHome logging macros.
- Keep probing code passive unless the config name and docs clearly say it drives a bus.

## Hardware Safety

Treat the bare firmware as the baseline until pins are confirmed. GPIO probe configs configure listed pins as inputs only, but do not add flash pins GPIO6-GPIO11 or pins currently used by active Ethernet/CAN configs. GPIO0 is a boot strap pin; only sample it when you know the board boots normally.

Do not enable CAN `NORMAL` mode or Ethernet until the transceiver/PHY pins and power/reset lines have been traced.

