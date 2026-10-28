"""Check that both public ESPHome schemas apply the calibration validation."""

from pathlib import Path
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("mode", ["discovery", "sensor"])
@pytest.mark.parametrize("settings,message", [
    ("min_value: 125\nmax_value: 10", "max_value must be greater than min_value"),
    ("length: 2", "length must be a finite number greater than 2 cm"),
    ("full_height: 2", "full_height must be a finite number greater than 2 cm"),
    ("length: 15\nfull_height: 20", "full_height must not exceed length"),
    ("full_height: .nan", "value must be at least 2"),
])
def test_invalid_configuration_fails_before_compiling(tmp_path, mode, settings, message):
    component_path = (ROOT / "components").as_posix()
    config = (
        "esphome:\n  name: test-waterlevel\n"
        "esp32:\n  board: esp32dev\n  framework:\n    type: esp-idf\n"
        f"external_components:\n  - source: '{component_path}'\n"
        "    components: [ewaterlevel_ble]\nesp32_ble_tracker:\n"
    )
    if mode == "discovery":
        config += "ewaterlevel_ble:\n" + "\n".join("  " + line for line in settings.splitlines())
    else:
        config += 'sensor:\n  - platform: ewaterlevel_ble\n    mac_address: "AA:BB:CC:DD:EE:FF"\n'
        config += "\n".join("    " + line for line in settings.splitlines())
    filename = tmp_path / "invalid.yaml"
    filename.write_text(config + "\n", encoding="utf-8")
    result = subprocess.run(
        [sys.executable, "-m", "esphome", "config", str(filename)],
        cwd=ROOT, text=True, encoding="utf-8", errors="replace", capture_output=True, timeout=60,
    )
    assert result.returncode != 0
    assert message in result.stdout + result.stderr
