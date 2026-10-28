#include "components/ewaterlevel_ble/ewaterlevel_packet.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace esphome::ewaterlevel_ble;

static void expect_close(float actual, float expected) {
  assert(std::fabs(actual - expected) < 0.001f);
}

int main(int argc, char **argv) {
  assert(argc == 2);
  std::ifstream fixtures(argv[1]);
  assert(fixtures.is_open());
  std::string hex;
  unsigned battery_mv;
  unsigned count = 0;
  std::vector<uint8_t> bytes;
  ewaterlevel_data packet{};
  while (fixtures >> hex >> battery_mv) {
    bytes.clear();
    for (size_t i = 0; i < hex.size(); i += 2) {
      bytes.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    }
    assert(decode_advertisement(bytes.data(), bytes.size(), packet));
    assert(packet.version_hw_high == 3 && packet.version_hw_low == 0);
    assert(packet.version_sw_high == 1 && packet.version_sw_low == 0);
    expect_close(packet.read_battery_voltage(), battery_mv / 1000.0f);
    expect_close(packet.read_long_pin_length(), 37.5f);
    expect_close(packet.read_short_pin_length(), 0.0f);
    expect_close(packet.read_value(), 0.0f);
    if (count == 0) expect_close(packet.read_counter(), 163.56f);
    ++count;
  }
  assert(count == 17);

  // The old marker is a synthetic compatibility case, not a second hardware capture.
  bytes[3] = 0x17;
  assert(decode_advertisement(bytes.data(), bytes.size(), packet));
  bytes[3] = 0x19;
  for (size_t length = 0; length < bytes.size(); ++length) {
    assert(!decode_advertisement(bytes.data(), length, packet));
  }
  assert(!decode_advertisement(nullptr, 30, packet));
  bytes.push_back(0);
  assert(!decode_advertisement(bytes.data(), bytes.size(), packet));
  bytes.pop_back();
  for (size_t index = 0; index < 9; ++index) {
    const auto original = bytes[index];
    bytes[index] = 0;
    assert(!decode_advertisement(bytes.data(), bytes.size(), packet));
    bytes[index] = original;
  }
  assert(decode_advertisement(bytes.data(), bytes.size(), packet));

  packet.state_a = 1;
  packet.value = 2000;  // Raw value 10: lower calibration point at 2 cm.
  expect_close(calculate_water_height(packet, 10, 125, 37.5f), 2.0f);
  packet.value = 25000;  // Raw value 125: full pin immersion.
  expect_close(calculate_water_height(packet, 10, 125, 37.5f), 37.5f);
  packet.value = 13500;  // Raw value 67.5: halfway between calibration points.
  expect_close(calculate_water_height(packet, 10, 125, 37.5f), 19.75f);
  expect_close(calculate_water_height(packet, 10, 125, 30.0f), 16.0f);
  packet.value = 0;
  expect_close(calculate_water_height(packet, 10, 125, 37.5f), 0.0f);
  packet.value = 40000;
  expect_close(calculate_water_height(packet, 10, 125, 37.5f), 37.5f);

  assert(std::isnan(calculate_water_height(packet, 10, 10, 37.5f)));
  assert(std::isnan(calculate_water_height(packet, 125, 10, 37.5f)));
  assert(std::isnan(calculate_water_height(packet, NAN, 125, 37.5f)));
  assert(std::isnan(calculate_water_height(packet, 10, INFINITY, 37.5f)));
  for (float length : {0.0f, 1.0f, 2.0f, NAN, INFINITY}) {
    assert(std::isnan(calculate_water_height(packet, 10, 125, length)));
    assert(std::isnan(calculate_water_level(0, length)));
  }
  packet.state_a = 0;
  expect_close(calculate_water_height(packet, 10, 125, 37.5f), 0.0f);
  packet.state_a = 6;
  assert(std::isnan(calculate_water_height(packet, 10, 125, 37.5f)));
  expect_close(calculate_water_level(37.5f, 37.5f), 100.0f);
  expect_close(calculate_water_level(0, 37.5f), 0.0f);
  assert(std::isnan(calculate_water_level(NAN, 37.5f)));
  // A 30 cm pin with a full-tank immersion of 20 cm must report 20 cm / 100%.
  packet.state_a = 1;
  packet.value = 25000;
  float height = calculate_water_height(packet, 10, 125, 30, 20);
  expect_close(height, 20);
  expect_close(calculate_water_level(height, 20), 100);
  packet.value = 2000;
  height = calculate_water_height(packet, 10, 125, 30, 20);
  expect_close(height, 2);
  expect_close(calculate_water_level(height, 20), 10);  // Lower point is not empty.
  packet.value = 13500;
  height = calculate_water_height(packet, 10, 125, 30, 20);
  expect_close(height, 11);
  expect_close(calculate_water_level(height, 20), 55);
  packet.value = 40000;
  height = calculate_water_height(packet, 10, 125, 30, 20);
  expect_close(height, 30);  // Height stays physical; percentage is capped at 100.
  expect_close(calculate_water_level(height, 20), 100);
  for (float full_height : {0.0f, 2.0f, 30.1f, INFINITY}) {
    assert(std::isnan(calculate_water_height(packet, 10, 125, 30, full_height)));
  }
  // Omitted full_height keeps existing calibration and transmitted-length fallback.
  expect_close(calculate_water_height(packet, 10, 125, 30, NAN), 30);
  packet.state_a = 6;
  assert(std::isnan(calculate_water_height(packet, 10, 125, 30, 20)));
  packet.state_a = 0;
  expect_close(calculate_water_height(packet, 10, 125, 30, 20), 0);
  // Bad geometry is unknown even if the device reports an empty/too-low state.
  assert(std::isnan(calculate_water_height(packet, 10, 125, 15, 20)));
  std::cout << "PASS: 17 captured packets, legacy marker, malformed packets and calibration boundaries\n";
}
