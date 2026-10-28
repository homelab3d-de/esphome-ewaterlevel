# Development and testing

This fork builds on [Fabian-Schmidt/esphome-ewaterlevel](https://github.com/Fabian-Schmidt/esphome-ewaterlevel).
Changes are maintained by homelab3d.de under the original GPL-3.0 license.

## Local checks

Use Python 3.12 and install the development dependencies in a virtual environment:

```sh
python -m pip install -r requirements-dev.txt
python tools/run_tests.py
```

The runner compiles and executes the same C++ packet decoder used by the ESP32.
It also checks the calibration validation, including both public ESPHome configuration schemas.
The compiler is supplied by the `ziglang` package, so no global C++ toolchain is required for these tests.

Copy `examples/secrets.yaml.example` to `examples/secrets.yaml`, then run:

```sh
esphome config examples/discovery.yaml
esphome config examples/calibration.yaml
esphome config examples/home-assistant.yaml
esphome config examples/volume.yaml
esphome compile examples/home-assistant.yaml
esphome compile examples/volume.yaml
```

All examples load the component from the checkout's `components` directory.
Their example calibration values are not a calibration for a specific tank.
The firmware build is a compilation check. Do not flash firmware built with example Wi-Fi credentials.

## Packet fixtures and supported layouts

`tests/fixtures/hw3-sw1.txt` contains 17 real advertisements captured from a HW 3.0 / SW 1.0
sensor on 2026-10-01. Each line contains a raw hexadecimal advertisement and its expected
battery voltage in millivolts. MAC addresses, Wi-Fi information and log timestamps are omitted.

These captures have a 30-byte advertisement, the `WTRL` signature, the `0x19` marker,
a transmitted pin length of 37.5 cm and a dry raw value of zero.
The decoder preserves the upstream `0x17` marker as a synthetic compatibility test;
this does not constitute a hardware test of older sensor versions.

The counter keeps the upstream big-endian interpretation. Battery voltage, raw value and
pin lengths use little-endian 16-bit values. Do not shorten the long-pin field to 8 bits:
375 mm requires both bytes.

The tests cover rejected lengths and signatures, calibration endpoints and midpoint,
shortened pins, overflow-limited full height, invalid calibration, invalid sensor states,
and zero/NaN/infinite geometry. They check that 30 cm pins with `full_height: 20`
report 20 cm / 100% at the upper point, 2 cm / 10% at the lower point and
11 cm / 55% at the midpoint, without changing legacy full-pin calibration.
Invalid calibration or geometry produces an unknown value (`NaN`), rather than a false empty-tank reading.
The upstream empty/too-low state (`0`) continues to report zero; unsupported states report unknown.

## Hardware acceptance before a release

1. Confirm discovery and the printed MAC address with the intended ESPHome version.
2. Check that the raw value changes when the pins enter and leave clean water.
3. Install in the final tank position and level the vehicle. Record the empty and
   normal full raw values and use the empty/full percentage mapping.
4. Confirm that these endpoints give 0% and 100%, including a valid raw value of zero.
5. Confirm entity discovery in Home Assistant and recovery after ESP32 restart.
6. Remove the sensor battery and confirm that the example entities become unknown after five minutes.
7. Restore the battery and confirm that measurements recover automatically.
8. For the volume example, check the recorded quantities and timeout propagation
   to both derived entities. Check that out-of-range values are capped at empty/full.

The examples map raw values directly, without using the component's legacy `height`
and `level` outputs. When separately testing those legacy outputs, calibrate at 2 cm
immersion and at `full_height`, and compare against measured water heights.

Battery percentage retains the original linear estimate; use voltage for diagnostics.

## Continuous integration

The GitHub workflow runs the tests and an ESP32 ESP-IDF firmware build using ESPHome 2026.9.1.
It becomes active when the workflow is pushed and GitHub Actions is enabled for this fork.
A passing CI run does not replace the hardware acceptance checks above.
