from __future__ import annotations

from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.cpp_helpers import gpio_pin_expression

CODEOWNERS = []

CONF_PROBE_PINS = "probe_pins"

ecan_e02_ns = cg.esphome_ns.namespace("ecan_e02")
EcanE02Component = ecan_e02_ns.class_("EcanE02Component", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(EcanE02Component),
        cv.Optional(CONF_PROBE_PINS, default=[]): cv.ensure_list(
            pins.internal_gpio_input_pin_schema
        ),
    }
).extend(cv.polling_component_schema("30s"))


async def to_code(config):
    cg.add_global(ecan_e02_ns.using)
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    for pin_config in config[CONF_PROBE_PINS]:
        pin = await gpio_pin_expression(pin_config)
        cg.add(var.add_probe_pin(pin))

