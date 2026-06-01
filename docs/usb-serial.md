# USB serial troubleshooting

The ECAN-E02 board is flashed through an external WCH CH343-family USB-UART adapter. The CH343 is not onboard the ECAN-E02.

## Current local findings

On this host, `lsusb` did not show a WCH/QinHeng device (`1a86:*`) when checked. Because the CH343 is external, that means the USB-UART adapter itself was not enumerating in the current environment. A missing driver normally still leaves a USB device visible in `lsusb`; it just does not create `/dev/ttyUSB*` or `/dev/ttyACM*`.

The host kernel has the in-tree `ch341` driver available:

- module: `ch341`
- kernel: `6.14.0-1004-oem`
- supported common IDs include `1a86:7523`, `1a86:7522`, and `1a86:5523`

There is no in-tree `ch343` module on this host, and no blacklist entry was found for `ch341`, `usbserial`, `cdc_acm`, or `ch343`.

## Expected cases

### Device appears as `1a86:7523`, `1a86:7522`, or `1a86:5523`

The in-kernel `ch341` driver should bind and create a tty device, usually `/dev/ttyUSB0`.

Try:

```sh
sudo modprobe ch341
ls /dev/ttyUSB*
```

### Device appears as a CDC ACM serial interface

Some CH343 configurations are CDC ACM compatible. The kernel `cdc_acm` module should bind and create a tty device, usually `/dev/ttyACM0`.

Try:

```sh
sudo modprobe cdc_acm
ls /dev/ttyACM*
```

### Device appears as another `1a86:*` CH343 ID with no tty

The board may need WCH's CH343/CH34x vendor driver, especially if the product ID is not covered by the in-tree `ch341` driver.

WCH driver repositories:

- CH341/CH340 style driver: https://github.com/WCHSoftGroup/ch341ser_linux
- CH343 style driver: https://github.com/WCHSoftGroup/ch343ser_linux

General flow:

```sh
git clone https://github.com/WCHSoftGroup/ch343ser_linux.git
cd ch343ser_linux/driver
make
sudo make load
sudo make install
```

Use the vendor driver only after `lsusb` confirms the CH343 adapter is visible and the built-in `ch341` or `cdc_acm` driver did not bind.

## Container or sandbox note

This workspace may not expose host serial devices inside `/dev`. If the host sees `/dev/ttyUSB0` or `/dev/ttyACM0` but this workspace does not, pass the device through to the environment or run the flash command from a host shell:

```sh
./scripts/flash-bare.sh /dev/ttyUSB0
```

or:

```sh
./scripts/flash-bare.sh /dev/ttyACM0
```
