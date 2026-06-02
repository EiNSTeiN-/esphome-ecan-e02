from __future__ import annotations

from esphome import pins
import esphome.codegen as cg
from esphome.components.esp32 import include_builtin_idf_component
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_RX_PIN, CONF_TX_PIN
from esphome.cpp_helpers import gpio_pin_expression

CODEOWNERS = []

CONF_BIT_RATE = "bit_rate"
CONF_CAN_SELF_TEST = "can_self_test"
CONF_PROBE_PINS = "probe_pins"

CAN_SELF_TEST_BIT_RATES = {
    "25KBPS": 25,
    "50KBPS": 50,
    "100KBPS": 100,
    "125KBPS": 125,
    "250KBPS": 250,
    "500KBPS": 500,
    "800KBPS": 800,
    "1000KBPS": 1000,
}

ecan_e02_ns = cg.esphome_ns.namespace("ecan_e02")
EcanE02Component = ecan_e02_ns.class_("EcanE02Component", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(EcanE02Component),
        cv.Optional(CONF_PROBE_PINS, default=[]): cv.ensure_list(
            pins.internal_gpio_input_pin_schema
        ),
        cv.Optional(CONF_CAN_SELF_TEST): cv.Schema(
            {
                cv.Required(CONF_TX_PIN): pins.internal_gpio_output_pin_number,
                cv.Required(CONF_RX_PIN): pins.internal_gpio_input_pin_number,
                cv.Optional(CONF_BIT_RATE, default="500KBPS"): cv.enum(
                    CAN_SELF_TEST_BIT_RATES, upper=True
                ),
            }
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

    if can_self_test_config := config.get(CONF_CAN_SELF_TEST):
        cg.add_define("USE_ECAN_E02_CAN_SELF_TEST")
        include_builtin_idf_component("driver")
        include_builtin_idf_component("esp_driver_twai")
        cg.add(
            var.set_can_self_test(
                can_self_test_config[CONF_TX_PIN],
                can_self_test_config[CONF_RX_PIN],
                can_self_test_config[CONF_BIT_RATE],
            )
        )
