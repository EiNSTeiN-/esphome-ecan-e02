CONFIG ?= configs/ecan-e02-bare.yaml
PORT ?=
EXAMPLES := configs/ecan-e02-can-listen.yaml.example configs/ecan-e02-ethernet-rtl8201.yaml.example configs/ecan-e02-gpio-probe.yaml.example

.PHONY: config compile flash logs version usb-check watch-usb ch343-driver config-examples compile-can compile-ethernet clean

config:
	./scripts/esphome.sh config "$(CONFIG)"

compile:
	./scripts/esphome.sh compile "$(CONFIG)"

flash:
	@port="$(PORT)"; if [ -z "$$port" ]; then port="$$(./scripts/serial-port.sh)"; fi; ./scripts/esphome.sh upload "$(CONFIG)" --device "$$port"

logs:
	@port="$(PORT)"; if [ -z "$$port" ]; then port="$$(./scripts/serial-port.sh)"; fi; ./scripts/esphome.sh logs "$(CONFIG)" --device "$$port"

version:
	./scripts/esphome.sh version

usb-check:
	./scripts/usb-check.sh

watch-usb:
	./scripts/watch-usb.sh

ch343-driver:
	./scripts/ch343-driver.sh load

config-examples:
	@for config in $(EXAMPLES); do ./scripts/esphome.sh config "$$config"; done

compile-can:
	./scripts/esphome.sh compile configs/ecan-e02-can-listen.yaml.example

compile-ethernet:
	./scripts/esphome.sh compile configs/ecan-e02-ethernet-rtl8201.yaml.example

clean:
	rm -rf .esphome configs/.esphome
