#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

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

 protected:
  std::vector<GPIOPin *> probe_pins_{};
};

}  // namespace ecan_e02
}  // namespace esphome

