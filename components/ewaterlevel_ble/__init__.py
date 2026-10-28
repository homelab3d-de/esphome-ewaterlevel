import esphome.config_validation as cv

# Modified by homelab3d.de on 2026-10-01. See LICENSE.
from ._shared import (
    EWaterlevel_schema,
    EWaterlevel_to_code,
    validate_calibration,
)

CODEOWNERS = ["@Fabian-Schmidt", "@homelab3d-de"]

AUTO_LOAD = ["sensor"]
DEPENDENCIES = ["esp32_ble_tracker"]

CONFIG_SCHEMA = cv.All(EWaterlevel_schema, validate_calibration)


async def to_code(config):
    await EWaterlevel_to_code(config)
