#include "ecan_e02.h"

#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include <cinttypes>
#include <cstdio>
#include <string>

#ifdef USE_ESP32
#include "driver/gpio.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "esp_rom_sys.h"
#include "esp_system.h"
#endif

#if defined(USE_ESP32) && (defined(USE_ECAN_E02_CAN_SELF_TEST) || defined(USE_ECAN_E02_TWAI_STATUS))
#include "driver/twai.h"
#endif

namespace esphome {
namespace ecan_e02 {

static const char *const TAG = "ecan_e02";

#if defined(USE_ESP32) && defined(USE_ECAN_E02_MDIO_SCAN)
static void mdio_delay_() { esp_rom_delay_us(5); }

static void mdio_drive_low_(gpio_num_t mdio) {
  gpio_set_level(mdio, 0);
  gpio_set_direction(mdio, GPIO_MODE_OUTPUT);
}

static void mdio_release_(gpio_num_t mdio) {
  gpio_set_direction(mdio, GPIO_MODE_INPUT);
  gpio_set_pull_mode(mdio, GPIO_PULLUP_ONLY);
}

static void mdio_clock_(gpio_num_t mdc) {
  mdio_delay_();
  gpio_set_level(mdc, 1);
  mdio_delay_();
  gpio_set_level(mdc, 0);
  mdio_delay_();
}

static void mdio_write_bit_(gpio_num_t mdc, gpio_num_t mdio, bool bit) {
  if (bit) {
    mdio_release_(mdio);
  } else {
    mdio_drive_low_(mdio);
  }
  mdio_clock_(mdc);
}

static bool mdio_read_bit_(gpio_num_t mdc, gpio_num_t mdio) {
  mdio_release_(mdio);
  mdio_delay_();
  gpio_set_level(mdc, 1);
  mdio_delay_();
  bool bit = gpio_get_level(mdio) != 0;
  gpio_set_level(mdc, 0);
  mdio_delay_();
  return bit;
}

static void mdio_write_bits_(gpio_num_t mdc, gpio_num_t mdio, uint32_t value, uint8_t bits) {
  for (int8_t bit = bits - 1; bit >= 0; bit--) {
    mdio_write_bit_(mdc, mdio, (value >> bit) & 0x01);
  }
}

static uint16_t mdio_read_register_(gpio_num_t mdc, gpio_num_t mdio, uint8_t phy_addr, uint8_t reg_addr,
                                    bool *valid_turnaround) {
  gpio_set_level(mdc, 0);

  for (uint8_t i = 0; i < 32; i++) {
    mdio_write_bit_(mdc, mdio, true);
  }

  mdio_write_bits_(mdc, mdio, 0b01, 2);        // Start of frame.
  mdio_write_bits_(mdc, mdio, 0b10, 2);        // Read operation.
  mdio_write_bits_(mdc, mdio, phy_addr, 5);
  mdio_write_bits_(mdc, mdio, reg_addr, 5);

  uint16_t value = 0;
  bool ta_first = mdio_read_bit_(mdc, mdio);   // Usually high impedance and read high.
  bool ta_second = mdio_read_bit_(mdc, mdio);  // PHY should drive this turnaround bit low.
  if (!ta_first) {
    // Some observed RTL8201 reads drive the turnaround zero one clock earlier.
    *valid_turnaround = true;
    value = ta_second ? 1 : 0;
    for (uint8_t i = 1; i < 16; i++) {
      value = static_cast<uint16_t>((value << 1) | (mdio_read_bit_(mdc, mdio) ? 1 : 0));
    }
  } else if (!ta_second) {
    *valid_turnaround = true;
    for (uint8_t i = 0; i < 16; i++) {
      value = static_cast<uint16_t>((value << 1) | (mdio_read_bit_(mdc, mdio) ? 1 : 0));
    }
  } else {
    *valid_turnaround = false;
    for (uint8_t i = 0; i < 16; i++) {
      value = static_cast<uint16_t>((value << 1) | (mdio_read_bit_(mdc, mdio) ? 1 : 0));
    }
  }

  mdio_release_(mdio);
  return value;
}

static bool mdio_register_value_plausible_(uint16_t value) { return value != 0x0000 && value != 0xFFFF; }
#endif

#if defined(USE_ESP32) && (defined(USE_ECAN_E02_CAN_SELF_TEST) || defined(USE_ECAN_E02_TWAI_STATUS))
static bool get_twai_timing(uint32_t bit_rate_kbps, twai_timing_config_t *config) {
  switch (bit_rate_kbps) {
    case 25:
      *config = TWAI_TIMING_CONFIG_25KBITS();
      return true;
    case 50:
      *config = TWAI_TIMING_CONFIG_50KBITS();
      return true;
    case 100:
      *config = TWAI_TIMING_CONFIG_100KBITS();
      return true;
    case 125:
      *config = TWAI_TIMING_CONFIG_125KBITS();
      return true;
    case 250:
      *config = TWAI_TIMING_CONFIG_250KBITS();
      return true;
    case 500:
      *config = TWAI_TIMING_CONFIG_500KBITS();
      return true;
    case 800:
      *config = TWAI_TIMING_CONFIG_800KBITS();
      return true;
    case 1000:
      *config = TWAI_TIMING_CONFIG_1MBITS();
      return true;
    default:
      return false;
  }
}

static const char *twai_state_to_string(twai_state_t state) {
  switch (state) {
    case TWAI_STATE_STOPPED:
      return "STOPPED";
    case TWAI_STATE_RUNNING:
      return "RUNNING";
    case TWAI_STATE_BUS_OFF:
      return "BUS_OFF";
    case TWAI_STATE_RECOVERING:
      return "RECOVERING";
    default:
      return "UNKNOWN";
  }
}

static void log_twai_status(const char *context) {
  twai_status_info_t status = {};
  esp_err_t err = twai_get_status_info(&status);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "TWAI status %s unavailable: %s", context, esp_err_to_name(err));
    return;
  }

  ESP_LOGW(TAG,
           "TWAI status %s: state=%s txq=%" PRIu32 " rxq=%" PRIu32 " tx_err=%" PRIu32
           " rx_err=%" PRIu32 " tx_fail=%" PRIu32 " rx_missed=%" PRIu32 " rx_overrun=%" PRIu32
           " arb_lost=%" PRIu32 " bus_err=%" PRIu32,
           context, twai_state_to_string(status.state), status.msgs_to_tx, status.msgs_to_rx,
           status.tx_error_counter, status.rx_error_counter, status.tx_failed_count, status.rx_missed_count,
           status.rx_overrun_count, status.arb_lost_count, status.bus_error_count);
}
#endif

void EcanE02Component::setup() {
  for (auto *pin : this->probe_pins_) {
    pin->setup();
  }
  ESP_LOGI(TAG, "Ebyte ECAN-E02 bring-up component ready");

  if (this->can_self_test_enabled_) {
    this->can_self_test_ready_ = this->setup_can_self_test_();
  }

  if (this->mdio_scan_enabled_) {
    this->mdio_scan_ready_ = this->setup_mdio_scan_();
  }
}

void EcanE02Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Ebyte ECAN-E02:");

#ifdef USE_ESP32
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  ESP_LOGCONFIG(TAG, "  ESP32 chip model: %d", static_cast<int>(chip_info.model));
  ESP_LOGCONFIG(TAG, "  ESP32 chip revision: %d", chip_info.revision);
  ESP_LOGCONFIG(TAG, "  CPU cores: %u", chip_info.cores);
  ESP_LOGCONFIG(TAG, "  Reset reason: %d", static_cast<int>(esp_reset_reason()));

  uint32_t flash_size = 0;
  if (esp_flash_get_size(nullptr, &flash_size) == ESP_OK) {
    ESP_LOGCONFIG(TAG, "  Flash size: %" PRIu32 " bytes", flash_size);
  }

  uint8_t mac[6];
  if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
    ESP_LOGCONFIG(TAG, "  Base MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4],
                  mac[5]);
  }
#else
  ESP_LOGCONFIG(TAG, "  ESP32 diagnostics unavailable on this target");
#endif

  if (this->probe_pins_.empty()) {
    ESP_LOGCONFIG(TAG, "  Probe pins: none");
  } else {
    for (auto *pin : this->probe_pins_) {
      LOG_PIN("  Probe Pin: ", pin);
    }
  }

  if (this->can_self_test_enabled_) {
    ESP_LOGCONFIG(TAG, "  CAN self-test: tx=GPIO%u rx=GPIO%u bit_rate=%" PRIu32 "KBPS ready=%s",
                  this->can_self_test_tx_pin_, this->can_self_test_rx_pin_, this->can_self_test_bit_rate_kbps_,
                  TRUEFALSE(this->can_self_test_ready_));
  }

  ESP_LOGCONFIG(TAG, "  TWAI status monitor: %s", TRUEFALSE(this->twai_status_monitor_enabled_));

  if (this->mdio_scan_enabled_) {
    ESP_LOGCONFIG(TAG, "  MDIO scan: mdc=GPIO%u mdio=GPIO%u phy_addr=%u..%u batch=%u ready=%s",
                  this->mdio_scan_mdc_pin_, this->mdio_scan_mdio_pin_, this->mdio_scan_phy_addr_start_,
                  this->mdio_scan_phy_addr_end_, this->mdio_scan_phy_addr_batch_size_,
                  TRUEFALSE(this->mdio_scan_ready_));
    if (this->mdio_scan_reset_enabled_) {
      ESP_LOGCONFIG(TAG, "  MDIO scan reset: GPIO%u active_low=%s hold=%" PRIu32 "ms settle=%" PRIu32 "ms",
                    this->mdio_scan_reset_pin_, TRUEFALSE(this->mdio_scan_reset_active_low_),
                    this->mdio_scan_reset_hold_ms_, this->mdio_scan_reset_settle_ms_);
    }
  }
}

void EcanE02Component::update() {
  if (this->can_self_test_ready_) {
    this->run_can_self_test_();
  }

#if defined(USE_ESP32) && defined(USE_ECAN_E02_TWAI_STATUS)
  if (this->twai_status_monitor_enabled_) {
    log_twai_status("monitor");
  }
#endif

  if (this->mdio_scan_ready_) {
    this->run_mdio_scan_();
  }

  if (this->probe_pins_.empty()) {
    return;
  }

  std::string states;
  char item[16];
  for (size_t i = 0; i < this->probe_pins_.size(); i++) {
    std::snprintf(item, sizeof(item), "%u=%u", static_cast<unsigned>(i),
                  this->probe_pins_[i]->digital_read() ? 1U : 0U);
    if (!states.empty()) {
      states += " ";
    }
    states += item;
  }
  ESP_LOGI(TAG, "Probe states: %s", states.c_str());
}

bool EcanE02Component::setup_can_self_test_() {
#if !defined(USE_ESP32) || !defined(USE_ECAN_E02_CAN_SELF_TEST)
  ESP_LOGE(TAG, "CAN self-test is only available on ESP32 targets");
  return false;
#else
  twai_timing_config_t timing_config;
  if (!get_twai_timing(this->can_self_test_bit_rate_kbps_, &timing_config)) {
    ESP_LOGE(TAG, "Unsupported CAN self-test bit rate: %" PRIu32 "KBPS", this->can_self_test_bit_rate_kbps_);
    return false;
  }

  twai_general_config_t general_config =
      TWAI_GENERAL_CONFIG_DEFAULT(static_cast<gpio_num_t>(this->can_self_test_tx_pin_),
                                  static_cast<gpio_num_t>(this->can_self_test_rx_pin_), TWAI_MODE_NO_ACK);
  general_config.tx_queue_len = 4;
  general_config.rx_queue_len = 4;
  twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t err = twai_driver_install(&general_config, &timing_config, &filter_config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "CAN self-test driver install failed: %s", esp_err_to_name(err));
    return false;
  }

  err = twai_start();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "CAN self-test driver start failed: %s", esp_err_to_name(err));
    twai_driver_uninstall();
    return false;
  }

  ESP_LOGI(TAG, "CAN self-test ready in NO_ACK mode on tx=GPIO%u rx=GPIO%u at %" PRIu32 "KBPS",
           this->can_self_test_tx_pin_, this->can_self_test_rx_pin_, this->can_self_test_bit_rate_kbps_);
  log_twai_status("after start");
  return true;
#endif
}

void EcanE02Component::run_can_self_test_() {
#if defined(USE_ESP32) && defined(USE_ECAN_E02_CAN_SELF_TEST)
  uint32_t sequence = ++this->can_self_test_counter_;
  twai_message_t tx_message = {};
  tx_message.identifier = 0x321;
  tx_message.flags = TWAI_MSG_FLAG_SELF;
  tx_message.data_length_code = 8;
  tx_message.data[0] = 0x45;
  tx_message.data[1] = 0x43;
  tx_message.data[2] = 0x41;
  tx_message.data[3] = 0x4E;
  tx_message.data[4] = static_cast<uint8_t>((sequence >> 24) & 0xFF);
  tx_message.data[5] = static_cast<uint8_t>((sequence >> 16) & 0xFF);
  tx_message.data[6] = static_cast<uint8_t>((sequence >> 8) & 0xFF);
  tx_message.data[7] = static_cast<uint8_t>(sequence & 0xFF);

  esp_err_t err = twai_transmit(&tx_message, pdMS_TO_TICKS(100));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "CAN self-test tx failed seq=%" PRIu32 ": %s", sequence, esp_err_to_name(err));
    log_twai_status("after tx failure");
    return;
  }

  twai_message_t rx_message = {};
  err = twai_receive(&rx_message, pdMS_TO_TICKS(500));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "CAN self-test rx timeout seq=%" PRIu32 ": %s", sequence, esp_err_to_name(err));
    log_twai_status("after rx timeout");
    return;
  }

  bool pass = rx_message.identifier == tx_message.identifier && rx_message.data_length_code == tx_message.data_length_code;
  for (uint8_t i = 0; pass && i < tx_message.data_length_code; i++) {
    pass = rx_message.data[i] == tx_message.data[i];
  }

  if (pass) {
    ESP_LOGI(TAG, "CAN self-test PASS seq=%" PRIu32 " id=0x%03" PRIX32 " data=%02X %02X %02X %02X %02X %02X %02X %02X",
             sequence, rx_message.identifier, rx_message.data[0], rx_message.data[1], rx_message.data[2],
             rx_message.data[3], rx_message.data[4], rx_message.data[5], rx_message.data[6], rx_message.data[7]);
  } else {
    ESP_LOGE(TAG, "CAN self-test FAIL seq=%" PRIu32 " rx_id=0x%03" PRIX32 " rx_len=%u", sequence,
             rx_message.identifier, rx_message.data_length_code);
  }
#endif
}

bool EcanE02Component::setup_mdio_scan_() {
#if !defined(USE_ESP32) || !defined(USE_ECAN_E02_MDIO_SCAN)
  ESP_LOGE(TAG, "MDIO scan is only available on ESP32 targets");
  return false;
#else
  if (this->mdio_scan_phy_addr_start_ > this->mdio_scan_phy_addr_end_ ||
      this->mdio_scan_phy_addr_end_ > 31) {
    ESP_LOGE(TAG, "Invalid MDIO scan PHY address range: %u..%u", this->mdio_scan_phy_addr_start_,
             this->mdio_scan_phy_addr_end_);
    return false;
  }

  gpio_num_t mdc = static_cast<gpio_num_t>(this->mdio_scan_mdc_pin_);
  gpio_num_t mdio = static_cast<gpio_num_t>(this->mdio_scan_mdio_pin_);
  gpio_set_direction(mdc, GPIO_MODE_OUTPUT);
  gpio_set_level(mdc, 0);
  gpio_set_drive_capability(mdc, GPIO_DRIVE_CAP_0);
  mdio_release_(mdio);

  if (this->mdio_scan_reset_enabled_) {
    gpio_num_t reset = static_cast<gpio_num_t>(this->mdio_scan_reset_pin_);
    int asserted = this->mdio_scan_reset_active_low_ ? 0 : 1;
    int deasserted = this->mdio_scan_reset_active_low_ ? 1 : 0;
    gpio_set_direction(reset, GPIO_MODE_OUTPUT);
    gpio_set_drive_capability(reset, GPIO_DRIVE_CAP_0);
    gpio_set_level(reset, asserted);
    delay(this->mdio_scan_reset_hold_ms_);
    gpio_set_level(reset, deasserted);
    delay(this->mdio_scan_reset_settle_ms_);
    App.feed_wdt();
    ESP_LOGI(TAG, "MDIO scan reset released on GPIO%u", this->mdio_scan_reset_pin_);
  }

  this->mdio_scan_next_phy_addr_ = this->mdio_scan_phy_addr_start_;
  this->mdio_scan_hits_this_cycle_ = 0;
  this->mdio_scan_valid_ta_this_cycle_ = 0;

  ESP_LOGI(TAG, "MDIO scan ready on mdc=GPIO%u mdio=GPIO%u addr=%u..%u batch=%u", this->mdio_scan_mdc_pin_,
           this->mdio_scan_mdio_pin_, this->mdio_scan_phy_addr_start_, this->mdio_scan_phy_addr_end_,
           this->mdio_scan_phy_addr_batch_size_);
  return true;
#endif
}

void EcanE02Component::run_mdio_scan_() {
#if defined(USE_ESP32) && defined(USE_ECAN_E02_MDIO_SCAN)
  gpio_num_t mdc = static_cast<gpio_num_t>(this->mdio_scan_mdc_pin_);
  gpio_num_t mdio = static_cast<gpio_num_t>(this->mdio_scan_mdio_pin_);
  for (uint8_t scanned = 0; scanned < this->mdio_scan_phy_addr_batch_size_; scanned++) {
    uint8_t phy_addr = this->mdio_scan_next_phy_addr_;
    bool valid_id1 = false;
    bool valid_id2 = false;
    uint16_t phy_id1 = mdio_read_register_(mdc, mdio, phy_addr, 2, &valid_id1);
    uint16_t phy_id2 = mdio_read_register_(mdc, mdio, phy_addr, 3, &valid_id2);
    if (valid_id1 || valid_id2) {
      this->mdio_scan_valid_ta_this_cycle_++;
      bool valid_bmcr = false;
      bool valid_bmsr = false;
      uint16_t bmcr = mdio_read_register_(mdc, mdio, phy_addr, 0, &valid_bmcr);
      uint16_t bmsr = mdio_read_register_(mdc, mdio, phy_addr, 1, &valid_bmsr);

      if (valid_id1 && valid_id2 && mdio_register_value_plausible_(phy_id1) &&
          mdio_register_value_plausible_(phy_id2)) {
        this->mdio_scan_hits_this_cycle_++;
        ESP_LOGI(TAG, "MDIO PHY addr=%u bmcr=0x%04X%s bmsr=0x%04X%s phy_id=0x%04X:0x%04X", phy_addr, bmcr,
                 valid_bmcr ? "" : "?", bmsr, valid_bmsr ? "" : "?", phy_id1, phy_id2);
      } else {
        ESP_LOGW(TAG,
                 "MDIO turnaround addr=%u bmcr=0x%04X%s bmsr=0x%04X%s phy_id=0x%04X%s:0x%04X%s",
                 phy_addr, bmcr, valid_bmcr ? "" : "?", bmsr, valid_bmsr ? "" : "?", phy_id1,
                 valid_id1 ? "" : "?", phy_id2, valid_id2 ? "" : "?");
      }
    }

    if (phy_addr >= this->mdio_scan_phy_addr_end_) {
      if (this->mdio_scan_hits_this_cycle_ == 0) {
        ESP_LOGW(TAG,
                 "MDIO scan found no plausible PHY on mdc=GPIO%u mdio=GPIO%u addr=%u..%u valid_ta_addrs=%u",
                 this->mdio_scan_mdc_pin_, this->mdio_scan_mdio_pin_, this->mdio_scan_phy_addr_start_,
                 this->mdio_scan_phy_addr_end_, this->mdio_scan_valid_ta_this_cycle_);
      }
      this->mdio_scan_next_phy_addr_ = this->mdio_scan_phy_addr_start_;
      this->mdio_scan_hits_this_cycle_ = 0;
      this->mdio_scan_valid_ta_this_cycle_ = 0;
      break;
    }

    this->mdio_scan_next_phy_addr_ = static_cast<uint8_t>(phy_addr + 1);
    App.feed_wdt();
  }
#endif
}

}  // namespace ecan_e02
}  // namespace esphome
