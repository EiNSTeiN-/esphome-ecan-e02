#include "ecan_e02.h"

#include "esphome/core/log.h"

#include <cinttypes>
#include <cstdio>
#include <string>

#ifdef USE_ESP32
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "esp_system.h"
#endif

namespace esphome {
namespace ecan_e02 {

static const char *const TAG = "ecan_e02";

void EcanE02Component::setup() {
  for (auto *pin : this->probe_pins_) {
    pin->setup();
  }
  ESP_LOGI(TAG, "ECAN-E02 bring-up component ready");
}

void EcanE02Component::dump_config() {
  ESP_LOGCONFIG(TAG, "ECAN-E02:");

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
}

void EcanE02Component::update() {
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

}  // namespace ecan_e02
}  // namespace esphome

