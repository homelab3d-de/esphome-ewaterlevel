# E-Trailer E-Waterlevel for ESPHome

Read E-Trailer E-Waterlevel sensors directly with an ESP32
and expose their measurements to Home Assistant.

## About this fork

This project is a fork of
[Fabian-Schmidt/esphome-ewaterlevel](https://github.com/Fabian-Schmidt/esphome-ewaterlevel),
maintained by homelab3d.de.

The upstream version required changes to work with our ESPHome
2026.9.1 setup and HW 3.0 / SW 1.0 sensor. This fork continues
development to address those compatibility issues and simplify setup.

Credit for the original implementation belongs to Fabian Schmidt.
This fork retains the original GPL-3.0 license.

## Changes in this fork

- Updated Bluetooth scan-result handling for ESPHome 2026.9.1.
- Replaced deprecated MAC-address formatting calls.
- Added decoding for the 0x19 advertisement format observed on
  HW 3.0 / SW 1.0 sensors while retaining the original 0x17 format.
- Replaced packed-structure casting with explicit packet decoding
  and checks for packet length and signature.
- Corrected MAC-address formatting in configuration logs.
- Added calibration validation and handling of invalid measurements.
- Added `full_height` to calibrate 100% at the normal full-tank level, independently
  of pin length, including tanks limited by an overflow.
- Added simple empty/full percentage calibration and an optional five-point volume
  example for irregular tanks using ESPHome filters.
- Added automated decoder and configuration tests, including
  17 captured sensor advertisements.
- Added a GitHub Actions workflow for tests and an ESP32 firmware build.
- Added setup instructions and configuration examples with timeouts
  for missing sensor readings.

## Guides

- [Setup guide](docs/setup.md)
- [Optional volume calibration](docs/volume-calibration.md)
- [Development and testing](docs/testing.md)
- [German step-by-step tutorial on homelab3d.de](https://homelab3d.de/e-trailer-waterlevel-home-assistant)

## 1. Find Sensor

1. Run ESP Home with `ewaterlevel_ble` component.
2. Connect with log and scan for result like `E-Waterlevel SENSOR FOUND: AA:BB:CC:DD:EE:FF`.
3. Take note of Sensor Mac address and continue to step 2.

```yaml
external_components:
  - source: github://homelab3d-de/esphome-ewaterlevel@main
    components: ["ewaterlevel_ble"]

esp32_ble_tracker:
  scan_parameters:
    interval: 10ms
    window: 10ms
    active: false

ewaterlevel_ble:
```

[Discovery example (local checkout)](examples/discovery.yaml)

## 2. Calibration

Install the sensor in its final position and level the vehicle.
Record its raw `Value` with the tank empty, then at the normal full level limited
by the overflow. These two points map to 0% and 100% for a simple estimate.
No 2 cm immersion measurement is required for this method.

For an irregular tank, add about three intermediate points during the same fill,
for example 10, 20 and 30 L for a tank of about 40 L. Record the actual full quantity.

- [Setup guide](docs/setup.md)
- [Raw-value example (local checkout)](examples/calibration.yaml)
- [Irregular tank calibration](docs/volume-calibration.md)

## 3. Sensor usage

Use a raw E-Waterlevel sensor and map its reading to percentage with a copy sensor.
The MAC address and both raw calibration values below are examples; replace them.
Keep the external component, Bluetooth, Wi-Fi, API and OTA settings already configured.

```yaml
sensor:
  - platform: ewaterlevel_ble
    mac_address: "AA:BB:CC:DD:EE:FF"
    value:
      id: water_level_raw
      name: Raw Value
      filters:
        - timeout: 5min
    battery_voltage:
      name: Battery Voltage
      filters:
        - timeout: 5min
    battery_level:
      name: Battery
      filters:
        - timeout: 5min

  - platform: copy
    source_id: water_level_raw
    id: water_level_percent
    name: Water Level
    unit_of_measurement: "%"
    device_class: ""
    state_class: measurement
    entity_category: ""
    accuracy_decimals: 0
    filters:
      - calibrate_linear:
          method: exact
          datapoints:
            - 0.0 -> 0.0
            - 125.0 -> 100.0
      - lambda: |-
          if (!std::isfinite(x)) return NAN;
          return x < 0.0f ? 0.0f : (x > 100.0f ? 100.0f : x);
```

- [Empty/full example (local checkout)](examples/home-assistant.yaml)
- [Alternative volume example (local checkout)](examples/volume.yaml)

The original `height` and `level` outputs below remain available for existing users.
Their height calibration is separate from the raw-value mapping shown above.
In particular, do not use an empty-tank raw value as the original `min_value`.

Configuration variables:
------------------------

- **mac_address** (**Required**, MAC Address): The MAC address of the E-Waterlevel device.

- **min_value** (*Optional*, default `10`): Raw value at 2 cm pin immersion, not at empty. Replace the default with your measurement.

- **max_value** (*Optional*, default `125`): Raw value at the immersion configured by `full_height`. Replace the default with your full-tank measurement.

- **length** (*Optional*): Actual pin length in cm if shortened; otherwise use the transmitted length. Must be greater than 2 cm and at most 38.3 cm.

- **full_height** (*Optional*): Pin immersion in cm at the normal full-tank level, measured from the pin tips. Must be greater than 2 cm and no greater than the pin length (maximum 38.3 cm). If omitted, defaults to the pin length for compatibility with existing full-pin calibration.

- **height** (*Optional*): Calibrated immersion in cm, measured from the pin tips, not the tank floor. Readings below 2 cm are outside the calibrated range.

  - All options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).

- **level** (*Optional*): Height as a percentage of `full_height`, limited to 0–100%. This is not a volume percentage. Use the [volume example](docs/volume-calibration.md) for irregular tanks.

  - All options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).

- **battery_voltage** (*Optional*): The information for the battery voltage sensor.

  - All options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).

- **battery_level** (*Optional*): The information for the battery percentage sensor. Sensor uses a standard CR2032 battery.

  - All options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).

- **time** (*Optional*): The submitted timestamp from the sensor for diagnostics.

  - All options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).

- **value** (*Optional*): The raw measured value from the sensor for calibration or diagnostics.

  - All options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).
