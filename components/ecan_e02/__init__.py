from __future__ import annotations

from esphome import pins
import esphome.codegen as cg
from esphome.components.esp32 import include_builtin_idf_component
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_RESET_PIN, CONF_RX_PIN, CONF_TX_PIN
from esphome.cpp_helpers import gpio_pin_expression

CODEOWNERS = []

CONF_BIT_RATE = "bit_rate"
CONF_CAN_SELF_TEST = "can_self_test"
CONF_MDC_PIN = "mdc_pin"
CONF_MDIO_PIN = "mdio_pin"
CONF_MDIO_SCAN = "mdio_scan"
CONF_PHY_ADDR_BATCH_SIZE = "phy_addr_batch_size"
CONF_PHY_ADDR_END = "phy_addr_end"
CONF_PHY_ADDR_START = "phy_addr_start"
CONF_PROBE_PINS = "probe_pins"
CONF_RESET_ACTIVE_LOW = "reset_active_low"
CONF_RESET_HOLD_MS = "reset_hold_ms"
CONF_RESET_SETTLE_MS = "reset_settle_ms"

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

MDIO_SCAN_DISALLOWED_PINS = {
    6,
    7,
    8,
    9,
    10,
    11,
    16,
    17,
}


def validate_mdio_scan_pin(value):
    pin = pins.internal_gpio_output_pin_number(value)
    if pin in MDIO_SCAN_DISALLOWED_PINS:
        raise cv.Invalid(
            f"GPIO{pin} is reserved for flash or confirmed Ebyte ECAN-E02 board functions; "
            "trace the RTL8201 MDC/MDIO pins to a different ESP32 GPIO before using this scanner"
        )
    return pin

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
        cv.Optional(CONF_MDIO_SCAN): cv.Schema(
            {
                cv.Required(CONF_MDC_PIN): validate_mdio_scan_pin,
                cv.Required(CONF_MDIO_PIN): validate_mdio_scan_pin,
                cv.Optional(CONF_PHY_ADDR_START, default=0): cv.int_range(
                    min=0, max=31
                ),
                cv.Optional(CONF_PHY_ADDR_END, default=31): cv.int_range(
                    min=0, max=31
                ),
                cv.Optional(CONF_PHY_ADDR_BATCH_SIZE, default=1): cv.int_range(
                    min=1, max=32
                ),
                cv.Optional(CONF_RESET_PIN): pins.internal_gpio_output_pin_number,
                cv.Optional(CONF_RESET_ACTIVE_LOW, default=True): cv.boolean,
                cv.Optional(CONF_RESET_HOLD_MS, default=10): cv.int_range(
                    min=0, max=1000
                ),
                cv.Optional(CONF_RESET_SETTLE_MS, default=100): cv.int_range(
                    min=0, max=5000
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

    if mdio_scan_config := config.get(CONF_MDIO_SCAN):
        cg.add_define("USE_ECAN_E02_MDIO_SCAN")
        include_builtin_idf_component("driver")
        cg.add(
            var.set_mdio_scan(
                mdio_scan_config[CONF_MDC_PIN],
                mdio_scan_config[CONF_MDIO_PIN],
                mdio_scan_config[CONF_PHY_ADDR_START],
                mdio_scan_config[CONF_PHY_ADDR_END],
                mdio_scan_config[CONF_PHY_ADDR_BATCH_SIZE],
            )
        )
        if CONF_RESET_PIN in mdio_scan_config:
            cg.add(
                var.set_mdio_scan_reset(
                    mdio_scan_config[CONF_RESET_PIN],
                    mdio_scan_config[CONF_RESET_ACTIVE_LOW],
                    mdio_scan_config[CONF_RESET_HOLD_MS],
                    mdio_scan_config[CONF_RESET_SETTLE_MS],
                )
            )
