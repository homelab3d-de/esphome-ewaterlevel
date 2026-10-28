#pragma once

// SPDX-License-Identifier: GPL-3.0-only
// Derived from Fabian-Schmidt/esphome-ewaterlevel.
// Modified by homelab3d.de on 2026-10-01: explicit packet decoding and guarded calibration.

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace esphome {
namespace ewaterlevel_ble {

struct ewaterlevel_data {
  uint16_t counter;
  uint8_t state_a;
  uint16_t battery_voltage;
  uint16_t value;
  uint8_t version_sw_low;
  uint8_t version_sw_high;
  uint8_t version_hw_low;
  uint8_t version_hw_high;
  uint8_t state_b;
  uint8_t value_high;
  uint16_t short_pin_length;
  uint16_t long_pin_length;
  uint8_t state_c;

  bool validate_state_a() const { return this->state_a > 0 && this->state_a < 6; }
  float read_counter() const { return 0.004f * this->counter; }
  float read_battery_voltage() const { return 0.001f * this->battery_voltage; }
  float read_value() const { return 0.005f * this->value; }
  float read_short_pin_length() const { return 0.1f * this->short_pin_length; }
  float read_long_pin_length() const { return 0.1f * this->long_pin_length; }
};

inline uint16_t read_le16(const uint8_t *bytes) {
  return static_cast<uint16_t>(bytes[0]) | (static_cast<uint16_t>(bytes[1]) << 8);
}

// Accept only the original 30-byte layout and the captured HW 3.0 variant.
// The legacy 0x17 marker is retained for compatibility with upstream.
// A different marker or packet length needs its own verified decoder.
inline bool decode_advertisement(const uint8_t *bytes, size_t length, ewaterlevel_data &data) {
  if (bytes == nullptr || length != 30) {
    return false;
  }
  if (bytes[0] != 0x02 || bytes[1] != 0x01 || bytes[2] != 0x06 ||
      (bytes[3] != 0x17 && bytes[3] != 0x19) || bytes[4] != 0xFF ||
      bytes[5] != 'W' || bytes[6] != 'T' || bytes[7] != 'R' || bytes[8] != 'L') {
    return false;
  }

  data.counter = (static_cast<uint16_t>(bytes[9]) << 8) | bytes[10];
  data.state_a = bytes[11];
  data.battery_voltage = read_le16(bytes + 14);
  data.value = read_le16(bytes + 16);
  data.version_sw_low = bytes[18];
  data.version_sw_high = bytes[19];
  data.version_hw_low = bytes[20];
  data.version_hw_high = bytes[21];
  data.state_b = bytes[22];
  data.value_high = bytes[23];
  data.short_pin_length = read_le16(bytes + 24);
  data.long_pin_length = read_le16(bytes + 26);
  data.state_c = bytes[28];
  return true;
}

inline float clamp_percentage(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 100.0f) return 100.0f;
  return value;
}

inline float calculate_water_height(const ewaterlevel_data &data, float min_value, float max_value,
                                    float pin_length, float full_height = NAN) {
  if (!std::isfinite(min_value) || !std::isfinite(max_value) || max_value <= min_value ||
      !std::isfinite(pin_length) || pin_length <= 2.0f) {
    return NAN;
  }
  // Existing configurations keep the upstream full-pin calibration.
  if (std::isnan(full_height)) full_height = pin_length;
  if (!std::isfinite(full_height) || full_height <= 2.0f || full_height > pin_length) {
    return NAN;
  }
  if (data.state_a == 0) {
    return 0.0f;
  }
  if (!data.validate_state_a()) {
    return NAN;
  }
  const float height = (data.read_value() - min_value) * (full_height - 2.0f) /
                           (max_value - min_value) + 2.0f;
  if (!std::isfinite(height)) return NAN;
  if (height < 0.0f) return 0.0f;
  if (height > pin_length) return pin_length;
  return height;
}

inline float calculate_water_level(float height, float full_height) {
  if (!std::isfinite(height) || !std::isfinite(full_height) || full_height <= 2.0f) return NAN;
  return clamp_percentage(height / full_height * 100.0f);
}

}  // namespace ewaterlevel_ble
}  // namespace esphome
