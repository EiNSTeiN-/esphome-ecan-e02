# USB serial troubleshooting

The ECAN-E02 board is flashed through an external WCH CH343-family USB-UART adapter. The CH343 is not onboard the ECAN-E02.

## Current local findings

On this host, `lsusb` did not show a WCH/QinHeng device (`1a86:*`) when checked. Because the CH343 is external, that means the USB-UART adapter itself was not enumerating in the current environment. A missing driver normally still leaves a USB device visible in `lsusb`; it just does not create `/dev/ttyUSB*` or `/dev/ttyACM*`.

Earlier kernel history did show an Espressif native USB device:

- vendor/product: `303a:1001`
- product: `USB JTAG/serial debug unit`
- driver: `cdc_acm`
- tty: `/dev/ttyACM0`
- serial: `34:85:18:07:A4:08`

That path uses the ESP32's native USB CDC/JTAG interface rather than the external CH343 USB-UART adapter. If this device is the board currently being flashed, use `/dev/ttyACM0`; if the external CH343 adapter is being used, expect a WCH/QinHeng USB device instead.

The host kernel has the in-tree `ch341` driver available:

- module: `ch341`
- kernel: `6.14.0-1004-oem`
- supported common IDs include `1a86:7523`, `1a86:7522`, and `1a86:5523`

There is no in-tree `ch343` module on this host, and no blacklist entry was found for `ch341`, `usbserial`, `cdc_acm`, or `ch343`.

The official WCH `ch343ser_linux` driver was also tested locally:

- repository: `https://github.com/WCHSoftGroup/ch343ser_linux`
- commit tested: `b705737356bb247bd6dd34e65ef56688aa35748c`
- kernel tested: `6.14.0-1004-oem`
- result: built successfully and loaded as the out-of-tree `ch343` module
- expected vendor-driver tty name: `/dev/ttyCH343USB0`

No `/dev/ttyCH343USB*` device appeared after loading the module because the CH343 adapter still was not visible in `lsusb`.

## Expected cases

### How to choose the driver

The two useful checks are the USB ID from `lsusb` and the created tty device:

- `303a:1001` with `/dev/ttyACM0`: Espressif native USB JTAG/serial, handled by `cdc_acm`.
- `1a86:7523`, `1a86:7522`, or `1a86:5523` with `/dev/ttyUSB0`: older WCH/QinHeng CH34x path, handled by the in-kernel `ch341` driver.
- `1a86:55d*` with `/dev/ttyACM0`: newer WCH CH342/CH343/CH910x family in CDC mode, handled by `cdc_acm`.
- `1a86:55d*` with no tty, or with a flaky CDC tty: try WCH's `ch343ser_linux` vendor driver.

WCH's CH343-family README says these chips are CDC-ACM compatible, but their vendor VCP driver can expose device-specific capabilities. It also notes that the generic `cdc_acm` driver must not already be bound to the WCH device when using the vendor VCP driver. That means `cdc_acm` is the first thing to try, and the vendor `ch343` driver is the fallback once `lsusb` confirms a WCH `1a86:*` device.

To capture the exact attach event:

```sh
./scripts/watch-usb.sh 60
```

To build or load the WCH vendor driver again:

```sh
./scripts/ch343-driver.sh build
./scripts/ch343-driver.sh load
./scripts/ch343-driver.sh status
```

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

### Device appears as Espressif `303a:1001`

This is the ESP32 native USB JTAG/serial interface. It also uses the kernel `cdc_acm` module and usually creates `/dev/ttyACM0`.

Try:

```sh
sudo modprobe cdc_acm
./scripts/flash-bare.sh /dev/ttyACM0
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

If `cdc_acm` bound to a WCH `1a86:55d*` device but the vendor driver is needed, unload the generic driver first:

```sh
sudo rmmod cdc_acm
sudo make load
```

Do not unload `cdc_acm` while using the Espressif native `303a:1001` `/dev/ttyACM0` path, because that is the working driver for that interface.

With the vendor driver loaded, a working adapter should appear as `/dev/ttyCH343USB0` or similar. Use that path explicitly if auto-detection does not pick it:

```sh
./scripts/flash-bare.sh /dev/ttyCH343USB0
```

## Container or sandbox note

This workspace may not expose host serial devices inside `/dev`. If the host sees `/dev/ttyUSB0` or `/dev/ttyACM0` but this workspace does not, pass the device through to the environment or run the flash command from a host shell:

```sh
./scripts/flash-bare.sh /dev/ttyUSB0
```

or:

```sh
./scripts/flash-bare.sh /dev/ttyACM0
```
