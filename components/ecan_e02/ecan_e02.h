#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

#include <cstdint>
#include <vector>

namespace esphome {
namespace ecan_e02 {

class EcanE02Component : public PollingComponent {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void add_probe_pin(GPIOPin *pin) { this->probe_pins_.push_back(pin); }
  void set_can_self_test(uint8_t tx_pin, uint8_t rx_pin, uint32_t bit_rate_kbps) {
    this->can_self_test_enabled_ = true;
    this->can_self_test_tx_pin_ = tx_pin;
    this->can_self_test_rx_pin_ = rx_pin;
    this->can_self_test_bit_rate_kbps_ = bit_rate_kbps;
  }
  void set_mdio_scan(uint8_t mdc_pin, uint8_t mdio_pin, uint8_t phy_addr_start, uint8_t phy_addr_end,
                     uint8_t phy_addr_batch_size) {
    this->mdio_scan_enabled_ = true;
    this->mdio_scan_mdc_pin_ = mdc_pin;
    this->mdio_scan_mdio_pin_ = mdio_pin;
    this->mdio_scan_phy_addr_start_ = phy_addr_start;
    this->mdio_scan_phy_addr_end_ = phy_addr_end;
    this->mdio_scan_phy_addr_batch_size_ = phy_addr_batch_size;
  }

 protected:
  std::vector<GPIOPin *> probe_pins_{};
  bool can_self_test_enabled_{false};
  bool can_self_test_ready_{false};
  uint8_t can_self_test_tx_pin_{0};
  uint8_t can_self_test_rx_pin_{0};
  uint32_t can_self_test_bit_rate_kbps_{500};
  uint32_t can_self_test_counter_{0};
  bool mdio_scan_enabled_{false};
  bool mdio_scan_ready_{false};
  uint8_t mdio_scan_mdc_pin_{0};
  uint8_t mdio_scan_mdio_pin_{0};
  uint8_t mdio_scan_phy_addr_start_{0};
  uint8_t mdio_scan_phy_addr_end_{31};
  uint8_t mdio_scan_phy_addr_batch_size_{1};
  uint8_t mdio_scan_next_phy_addr_{0};
  uint8_t mdio_scan_hits_this_cycle_{0};
  uint8_t mdio_scan_valid_ta_this_cycle_{0};

  bool setup_can_self_test_();
  void run_can_self_test_();
  bool setup_mdio_scan_();
  void run_mdio_scan_();
};

}  // namespace ecan_e02
}  // namespace esphome
