CONFIG ?= configs/ecan-e02-bare.yaml
PORT ?=

.PHONY: config compile flash logs version clean

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

clean:
	rm -rf .esphome

