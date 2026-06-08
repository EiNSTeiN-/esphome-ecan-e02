CONFIG ?= configs/ecan-e02-bare.yaml
BOARD_CONFIG ?= configs/ecan-e02.yaml
PORT ?=
EXAMPLES := configs/ecan-e02-can-listen.yaml.example configs/ecan-e02-ethernet-rtl8201.yaml.example configs/ecan-e02-gpio-probe.yaml.example

.PHONY: config compile flash logs version usb-check watch-usb ch343-driver config-examples config-board compile-board flash-board logs-board compile-can compile-ethernet clean

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

config-board:
	./scripts/esphome.sh config "$(BOARD_CONFIG)"

compile-board:
	./scripts/esphome.sh compile "$(BOARD_CONFIG)"

flash-board:
	@port="$(PORT)"; if [ -z "$$port" ]; then port="$$(./scripts/serial-port.sh)"; fi; ./scripts/esphome.sh upload "$(BOARD_CONFIG)" --device "$$port"

logs-board:
	@port="$(PORT)"; if [ -z "$$port" ]; then port="$$(./scripts/serial-port.sh)"; fi; ./scripts/esphome.sh logs "$(BOARD_CONFIG)" --device "$$port"

compile-can:
	./scripts/esphome.sh compile configs/ecan-e02-can-listen.yaml.example

compile-ethernet:
	./scripts/esphome.sh compile configs/ecan-e02-ethernet-rtl8201.yaml.example

clean:
	rm -rf .esphome configs/.esphome
