# Repository Guidelines

## Project Structure

This is an ESPHome firmware workspace for an ECAN-E02 board with an ESP32-U4WD target, an external CH343 USB-UART adapter for flashing/logging, an RTL8201 RMII Ethernet PHY, and an onboard CAN transceiver.

- `components/ecan_e02/` contains the local ESPHome external component used for board diagnostics and passive GPIO probing.
- `configs/` contains the main user-facing ESPHome YAML. `configs/ecan-e02.yaml` is the normal firmware target.
- `dev/configs/` contains bring-up, probing, bare recovery, and hardware validation YAMLs.
- `dev/docs/` contains hardware tracing notes. Update these as pins are confirmed.
- `dev/scripts/` contains development-only helper commands.
- `scripts/` contains local helper commands that run ESPHome through `uvx`.

## Build And Flash Commands

Use the helper script so ESPHome can run from `uvx` without installing globally:

```sh
./scripts/esphome.sh config configs/ecan-e02.yaml
./scripts/esphome.sh compile configs/ecan-e02.yaml
./scripts/flash.sh /dev/ttyACM0
./scripts/logs.sh
```

The scripts use `/tmp/uv-cache` and `/tmp/uv-tools` by default to avoid writing tool state into the home directory.

`make config`, `make compile`, `make flash`, and `make logs` wrap the same main firmware commands. Use `make dev-config DEV_CONFIG=dev/configs/<file>.yaml` and `make dev-compile DEV_CONFIG=dev/configs/<file>.yaml` for bring-up configs.

## Coding Style

Follow ESPHome component conventions:

- Python schema files define `CONFIG_SCHEMA` and `async def to_code(config)`.
- C++ components live beside the schema and use ESPHome logging macros.
- Keep probing code passive unless the config name and docs clearly say it drives a bus.

## Hardware Safety

Treat the main firmware as the normal user target. Use the bare firmware in `dev/configs/ecan-e02-bare.yaml` only for recovery or hardware bring-up. GPIO probe configs configure listed pins as inputs only, but do not add flash pins GPIO6-GPIO11 or pins currently used by active Ethernet/CAN configs. GPIO0 is a boot strap pin; only sample it when you know the board boots normally.

Do not enable CAN `NORMAL` mode on unknown hardware variants until the transceiver pins, power path, and bus termination are understood.
