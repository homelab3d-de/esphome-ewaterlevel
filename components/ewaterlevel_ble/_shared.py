# Modified by homelab3d.de on 2026-10-01. See LICENSE.
import math

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker
from esphome.const import (
    CONF_ID,
    CONF_LENGTH,
    CONF_MIN_VALUE,
    CONF_MAX_VALUE,
)

CONF_FULL_HEIGHT = "full_height"

ewaterlevel_ble_ns = cg.esphome_ns.namespace("ewaterlevel_ble")
EWaterlevel = ewaterlevel_ble_ns.class_(
    "EWaterLevel", cg.Component, esp32_ble_tracker.ESPBTDeviceListener
)

def validate_calibration(config):
    """Reject calibration values that cannot produce a meaningful water height."""
    minimum = config[CONF_MIN_VALUE]
    maximum = config[CONF_MAX_VALUE]
    if not math.isfinite(minimum) or not math.isfinite(maximum):
        raise cv.Invalid("min_value and max_value must be finite numbers")
    if maximum <= minimum:
        raise cv.Invalid("max_value must be greater than min_value", path=[CONF_MAX_VALUE])
    if CONF_LENGTH in config:
        length = config[CONF_LENGTH]
        if not math.isfinite(length) or length <= 2.0:
            raise cv.Invalid("length must be a finite number greater than 2 cm", path=[CONF_LENGTH])
    if CONF_FULL_HEIGHT in config:
        full_height = config[CONF_FULL_HEIGHT]
        if not math.isfinite(full_height) or full_height <= 2.0:
            raise cv.Invalid("full_height must be a finite number greater than 2 cm", path=[CONF_FULL_HEIGHT])
        if CONF_LENGTH in config and full_height > config[CONF_LENGTH]:
            raise cv.Invalid("full_height must not exceed length", path=[CONF_FULL_HEIGHT])
    return config


EWaterlevel_schema = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EWaterlevel),
            cv.Optional(CONF_MIN_VALUE, default=10): cv.float_range(min=0, max=250),
            cv.Optional(CONF_MAX_VALUE, default=125): cv.float_range(min=0, max=250),
            cv.Optional(CONF_LENGTH): cv.float_range(min=2, max=38.3),
            cv.Optional(CONF_FULL_HEIGHT): cv.float_range(min=2, max=38.3),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
)


async def EWaterlevel_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    if CONF_MIN_VALUE in config:
        cg.add(var.set_min_value(config[CONF_MIN_VALUE]))
    if CONF_MAX_VALUE in config:
        cg.add(var.set_max_value(config[CONF_MAX_VALUE]))
    if CONF_LENGTH in config:
        cg.add(var.set_length(config[CONF_LENGTH]))
    if CONF_FULL_HEIGHT in config:
        cg.add(var.set_full_height(config[CONF_FULL_HEIGHT]))

    return var
