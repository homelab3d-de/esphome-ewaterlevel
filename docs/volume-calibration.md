# Simple calibration for irregular tanks

For a tank with indentations or sloping walls, add approximately three intermediate
points to the empty/full calibration. This gives an approximate volume display
without measuring every litre or the water height.

## Record the measurements in one fill

1. Install the sensor in its final position and level the vehicle.
2. Drain the tank and record the raw `Value` for empty (0 L).
3. Add known quantities, for example a cumulative 10, 20 and 30 L for a tank of about
   40 L. Let the water settle and record the raw value at each point. Do not withdraw
   water during filling. Larger tanks can use larger steps.
4. At the normal full level, record the raw value and actual total amount in the tank.
   Exclude water lost through the overflow; do not block it.

If an unknown amount remains at the start, the table describes added water above
that empty reference, rather than the absolute contents. Water below the pin tips
cannot be distinguished from empty. Each higher calibration quantity needs a higher
raw value; duplicate raw values cannot represent different volumes.

## Example table

All values below are illustrative. Replace them with your own measurements.

| Raw Value | Water quantity (L) |
| --- | --- |
| 0.0 | 0.0 |
| 30.0 | 10.0 |
| 55.0 | 20.0 |
| 85.0 | 30.0 |
| 125.0 | 40.0 |

ESPHome connects neighbouring points with straight lines (`method: exact`). This
approximately accounts for changes in tank shape between those points.

## Configure the entities

Use [examples/volume.yaml](../examples/volume.yaml) as an alternative to the basic
[home-assistant example](../examples/home-assistant.yaml). Both include the same raw
sensor and battery configuration; neither requires height calibration.

For an existing device:

1. Keep the E-Waterlevel raw sensor with `id: water_level_raw` and `timeout: 5min`,
   plus the existing Wi-Fi, API, OTA and external component settings.
2. Replace the two-point percentage copy sensor with the two copy sensors below.
   Keep a single `sensor:` list and a single E-Waterlevel entry.
3. Replace all raw-value/litre pairs. Replace every `40.0f` in the two lambdas
   with your actual full quantity in litres.
4. Save, validate and install. Use **Water Level** for the dashboard and notifications;
   **Water Volume** provides the approximate litres.

```yaml
  - platform: copy
    source_id: water_level_raw
    id: water_volume
    name: Water Volume
    unit_of_measurement: L
    device_class: volume_storage
    state_class: measurement
    entity_category: ""
    accuracy_decimals: 0
    filters:
      - calibrate_linear:
          method: exact
          datapoints:
            - 0.0 -> 0.0
            - 30.0 -> 10.0
            - 55.0 -> 20.0
            - 85.0 -> 30.0
            - 125.0 -> 40.0
      - lambda: |-
          if (!std::isfinite(x)) return NAN;
          return x < 0.0f ? 0.0f : (x > 40.0f ? 40.0f : x);

  - platform: copy
    source_id: water_volume
    id: water_level_percent
    name: Water Level
    unit_of_measurement: "%"
    device_class: ""
    state_class: measurement
    entity_category: ""
    accuracy_decimals: 0
    filters:
      - lambda: return x * 100.0f / 40.0f;
```

Empty maps to 0 L and 0%. Values are limited to the empty/full range. Invalid or
missing source readings remain unknown in both derived entities, including after
the raw sensor's timeout. The display is an estimate for everyday use.
