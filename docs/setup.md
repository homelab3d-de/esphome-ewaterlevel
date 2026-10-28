# E-Waterlevel setup with ESPHome

Start with an ESP32 already configured in ESPHome and connected to Home Assistant.
Keep its existing Wi-Fi, API and OTA settings.

## Load the component from GitHub

ESPHome downloads the component directly from this repository.
You do not need to download or copy component files manually.

Add this to the device YAML, replacing an existing `external_components` source for this component:

```yaml
external_components:
  - source: github://homelab3d-de/esphome-ewaterlevel@main
    components: [ewaterlevel_ble]

esp32_ble_tracker:
  scan_parameters:
    interval: 10ms
    window: 10ms
    active: false

ewaterlevel_ble:
```

Merge these sections into an existing configuration without duplicating YAML keys.
Save and install the firmware. Open the logs and look for `E-Waterlevel SENSOR FOUND`.
Record the address printed after that message.

## Read the raw value

Replace the standalone `ewaterlevel_ble:` section with this sensor entry.
Replace the example MAC address with the address from the logs.

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
```


Install the firmware and check that `Value` changes when the pins are immersed in clean water.
Keep the electronics housing dry. The component log also contains legacy height/percentage calculations.
Use `Value` for this guide; those legacy calculations are not the new mapped percentage.

## Calibrate empty and full

Install the sensor in its final position according to the manufacturer's instructions
and level the vehicle. This setup estimates tank contents directly from the raw value;
no 2 cm immersion measurement or height calibration is required.

1. Drain the tank and record its raw `Value` as the empty reference (0%).
2. Fill to the normal maximum level, taking the overflow into account without blocking it.
3. Let the water settle and record the raw value as the full reference (100%).
4. Add the copy sensor below to the existing `sensor:` list. Replace `0.0` and `125.0`
   on the left of the arrows with your measured empty and full raw values.
5. Save, validate and install. The normal full tank should now show approximately 100%.

For a tank with an approximately constant horizontal cross-section, these two points
provide a simple estimate. Small amounts below the pin tips cannot be distinguished
from empty. A valid empty reading is 0%, not unknown.

```yaml
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

The raw sensor must have `id: water_level_raw`, as shown above. Keep only one `sensor:`
key and one E-Waterlevel entry. The full raw value must exceed the empty raw value.
The output is limited to 0-100%. Missing readings remain unknown through the raw
sensor's five-minute timeout; they are not converted to an empty tank.

Use [examples/home-assistant.yaml](../examples/home-assistant.yaml) for a local checkout.
If updating an older setup, replace its `height:` and `level:` outputs with this copy
sensor; `min_value`, `max_value`, `length` and `full_height` are not needed for this mapping.
The component still supports the original height calculation, but its `min_value`
means the raw reading at 2 cm immersion. Do not put an empty-tank reading there.

## Irregular tanks

During the same fill, add approximately three known intermediate quantities. For a
tank of about 40 L, record empty, 10 L, 20 L, 30 L and the actual normal full quantity.
Use larger steps for larger tanks. Record the cumulative amount remaining in the
tank, excluding water lost through the overflow.

The [volume guide](volume-calibration.md) maps these points directly to litres and
then to percentage. It replaces the two-point copy sensor; it does not require
calibrating centimetres first.

## Home Assistant

After installation, open **Settings > Devices & services > ESPHome** in Home Assistant.
Open the ESP32 device and check its new entities. If it has not been added yet, add the ESPHome
integration using the ESP32's IP address. Use its configured API encryption key if your existing device uses one.

The example timeout filters mark the entities unknown after five minutes without a measurement.
They recover when the sensor sends again. The battery percentage entity is an estimate.
