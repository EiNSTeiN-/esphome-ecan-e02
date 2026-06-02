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

 protected:
  std::vector<GPIOPin *> probe_pins_{};
  bool can_self_test_enabled_{false};
  bool can_self_test_ready_{false};
  uint8_t can_self_test_tx_pin_{0};
  uint8_t can_self_test_rx_pin_{0};
  uint32_t can_self_test_bit_rate_kbps_{500};
  uint32_t can_self_test_counter_{0};

  bool setup_can_self_test_();
  void run_can_self_test_();
};

}  // namespace ecan_e02
}  // namespace esphome
