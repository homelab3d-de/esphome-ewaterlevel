from pathlib import Path
import sys

import pytest
from esphome import config_validation as cv

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from components.ewaterlevel_ble._shared import validate_calibration


@pytest.mark.parametrize("minimum,maximum", [(10, 10), (125, 10), (float("nan"), 125), (10, float("inf"))])
def test_invalid_calibration_is_rejected(minimum, maximum):
    with pytest.raises(cv.Invalid):
        validate_calibration({"min_value": minimum, "max_value": maximum})


@pytest.mark.parametrize("length", [0, 1, 2, float("nan"), float("inf")])
def test_invalid_pin_length_is_rejected(length):
    with pytest.raises(cv.Invalid):
        validate_calibration({"min_value": 10, "max_value": 125, "length": length})


@pytest.mark.parametrize("length", [2.1, 30, 37.5, 38.3])
def test_valid_pin_length_is_preserved(length):
    config = {"min_value": 10, "max_value": 125, "length": length}
    assert validate_calibration(config) == config


def test_sensor_reported_length_can_be_used():
    config = {"min_value": 10, "max_value": 125}
    assert validate_calibration(config) == config


@pytest.mark.parametrize("height", [0, 1, 2, float("nan"), float("inf")])
def test_invalid_full_height_is_rejected(height):
    with pytest.raises(cv.Invalid):
        validate_calibration({"min_value": 10, "max_value": 125, "full_height": height})


def test_full_height_must_fit_configured_pins():
    with pytest.raises(cv.Invalid, match="full_height must not exceed length"):
        validate_calibration({"min_value": 10, "max_value": 125, "length": 15, "full_height": 20})


@pytest.mark.parametrize("settings", [{"length": 30, "full_height": 20}, {"length": 20, "full_height": 20}, {"full_height": 20}])
def test_valid_tank_calibration(settings):
    config = {"min_value": 10, "max_value": 125, **settings}
    assert validate_calibration(config) == config
