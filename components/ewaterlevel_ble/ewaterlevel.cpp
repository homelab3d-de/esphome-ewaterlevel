// Modified by homelab3d.de on 2026-10-01. See LICENSE.
#include "ewaterlevel.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32

namespace esphome {
namespace ewaterlevel_ble {

static const char *const TAG = "ewaterlevel_ble";

void EWaterLevel::dump_config() {
  ESP_LOGCONFIG(TAG, "E-Waterlevel");
  ESP_LOGCONFIG(TAG, "  Address: %s", format_ble_address_pretty(this->address_).c_str());
  ESP_LOGCONFIG(TAG, "  Min value: %.3f", this->min_value_);
  ESP_LOGCONFIG(TAG, "  Max value: %.3f", this->max_value_);
  if (!std::isnan(this->length_) && this->length_ > 0.0f) {
    ESP_LOGCONFIG(TAG, "  Length: %.1fcm", this->length_);
  }
  if (!std::isnan(this->full_height_)) {
    ESP_LOGCONFIG(TAG, "  Full tank immersion: %.1fcm", this->full_height_);
  }
  LOG_SENSOR("  ", "Time", this->time_);
  LOG_SENSOR("  ", "Value", this->value_);
  LOG_SENSOR("  ", "Height", this->height_);
  LOG_SENSOR("  ", "Level", this->level_);
  LOG_SENSOR("  ", "Battery Level", this->battery_level_);
  LOG_SENSOR("  ", "Battery Voltage", this->battery_voltage_);
}

/**
 * Parse all incoming BLE payloads to see if it is a matching BLE advertisement.
 * Currently this supports the following products:
 *
 * - E-Trailer E-Waterlevel
 *
 * The following points are used to identify a sensor:
 *
 * - Bluetooth data frame size
 * - Bluetooth data frame header
 */
bool EWaterLevel::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  char mac_address[18];
  device.address_str_to(mac_address);
  if (this->address_ != 0) {
    if (device.address_uint64() != this->address_) {
      return false;
    }
    ESP_LOGVV(TAG, "parse_device(): MAC address %s found.", mac_address);
  }

  const auto &param = device.get_scan_result();
  ewaterlevel_data decoded{};
  if (decode_advertisement(param.ble_adv, param.adv_data_len, decoded)) {
    const ewaterlevel_data *data = &decoded;
    const float pin_length = this->pin_length_(data);
    const float height = this->water_height_in_cm_(data);
    const float full_height = std::isnan(this->full_height_) ? pin_length : this->full_height_;
    const float level = calculate_water_level(height, full_height);

    if (this->address_ == 0) {
      ESP_LOGI(TAG, "E-Waterlevel SENSOR FOUND: %s", mac_address);
    }

    ESP_LOGV(TAG, "[%s] Sensor data: %s", mac_address,
             format_hex_pretty(param.ble_adv + 9, param.adv_data_len - 10).c_str());
    ESP_LOGD(TAG, "[%s] HW: V%u.%u SW: V%u.%u, ShortPin: %.1fcm, LongPin: %.1fcm", mac_address,
             data->version_hw_high, data->version_hw_low, data->version_sw_high, data->version_sw_low,
             data->read_short_pin_length(), data->read_long_pin_length());
    ESP_LOGD(TAG, "[%s] State_A: %s, State_B: %s, State_C: %s", mac_address,
             format_hex(&data->state_a, 1).c_str(), format_hex(&data->state_b, 1).c_str(),
             format_hex(&data->state_c, 1).c_str());
    ESP_LOGI(TAG, "[%s] Time: %.2f, Bat: %.3fV, Value: %.3f", mac_address, data->read_counter(),
             data->read_battery_voltage(), data->read_value());
    ESP_LOGI(TAG, "[%s] Waterlevel: %.1fcm, Percentage: %.1f%%", mac_address,
             height, level);

    if (this->time_ != nullptr) {
      this->time_->publish_state(data->read_counter());
    }

    if (this->value_ != nullptr) {
      // Preserve raw diagnostic logs, but do not feed unsupported states into volume filters.
      this->value_->publish_state(data->state_a == 0 || data->validate_state_a() ? data->read_value() : NAN);
    }

    if (this->height_ != nullptr) {
      this->height_->publish_state(height);
    }

    if (this->level_ != nullptr) {
      this->level_->publish_state(level);
    }

    if (this->battery_voltage_ != nullptr) {
      this->battery_voltage_->publish_state(data->read_battery_voltage());
    }

    if (this->battery_level_ != nullptr) {
      const auto battery_volt = data->read_battery_voltage();
      if (std::isnan(battery_volt)) {
        this->battery_level_->publish_state(NAN);
      } else {
        float percent = (battery_volt - 2.2f) / 0.65f * 100.0f;
        this->battery_level_->publish_state(clamp_percentage(percent));
      }
    }

    return true;
  }
  return false;
}

float EWaterLevel::water_height_in_cm_(const ewaterlevel_data *data) {
  return calculate_water_height(*data, this->min_value_, this->max_value_, this->pin_length_(data),
                                this->full_height_);
}

}  // namespace ewaterlevel_ble
}  // namespace esphome

#endif
