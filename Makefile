CONFIG ?= configs/ecan-e02.yaml
DEV_CONFIG ?= dev/configs/ecan-e02-bare.yaml
PORT ?=

.PHONY: config compile flash logs version usb-check ch343-driver dev-config dev-compile clean

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

ch343-driver:
	./scripts/ch343-driver.sh load

dev-config:
	./scripts/esphome.sh config "$(DEV_CONFIG)"

dev-compile:
	./scripts/esphome.sh compile "$(DEV_CONFIG)"

clean:
	rm -rf .esphome configs/.esphome dev/configs/.esphome
