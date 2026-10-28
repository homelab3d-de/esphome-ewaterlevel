#pragma once

// Modified by homelab3d.de on 2026-10-01. See LICENSE.
#include "ewaterlevel_packet.h"

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32

namespace esphome {
namespace ewaterlevel_ble {

static inline std::string format_ble_address_pretty(uint64_t address) {
  const uint8_t mac[6] = {
      static_cast<uint8_t>(address >> 40), static_cast<uint8_t>(address >> 32),
      static_cast<uint8_t>(address >> 24), static_cast<uint8_t>(address >> 16),
      static_cast<uint8_t>(address >> 8), static_cast<uint8_t>(address)};
  return format_mac_address_pretty(mac);
}

class EWaterLevel : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  void dump_config() override;

  void set_address(uint64_t val) { this->address_ = val; };
  void set_min_value(float val) { this->min_value_ = val; };
  void set_max_value(float val) { this->max_value_ = val; };
  void set_length(float val) { this->length_ = val; };
  void set_full_height(float val) { this->full_height_ = val; };
  void set_time(sensor::Sensor *val) { this->time_ = val; };
  void set_value(sensor::Sensor *val) { this->value_ = val; };
  void set_height(sensor::Sensor *val) { this->height_ = val; };
  void set_level(sensor::Sensor *val) { this->level_ = val; };
  void set_battery_voltage(sensor::Sensor *val) { this->battery_voltage_ = val; };
  void set_battery_level(sensor::Sensor *val) { this->battery_level_ = val; };

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

 protected:
  uint64_t address_{0};
  float min_value_{NAN};
  float max_value_{NAN};
  // Below 2cm no accurate measurement is possible.
  float length_{NAN};
  // Immersion at the normal full-tank level, measured from the pin tips.
  // Omitted: preserve upstream calibration at full pin immersion.
  float full_height_{NAN};
  sensor::Sensor *time_{nullptr};
  sensor::Sensor *value_{nullptr};
  sensor::Sensor *height_{nullptr};
  sensor::Sensor *level_{nullptr};
  sensor::Sensor *battery_voltage_{nullptr};
  sensor::Sensor *battery_level_{nullptr};

  float water_height_in_cm_(const ewaterlevel_data *data);

  float inline pin_length_(const ewaterlevel_data *data) {
    if (!std::isnan(this->length_) && this->length_ > 0.0f) {
      return this->length_;
    } else {
      return data->read_long_pin_length();
    }
  };
};

}  // namespace ewaterlevel_ble
}  // namespace esphome

#endif
