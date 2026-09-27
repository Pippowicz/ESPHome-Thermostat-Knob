"""Round thermostat UI for the Elecrow 240x240 ESP32-S3 knob."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, climate, display, font, image, light, number, select, switch, text
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32", "display", "psram"]
MULTI_CONF = True
round_thermostat_ns = cg.esphome_ns.namespace("round_thermostat")
RoundThermostat = round_thermostat_ns.class_("RoundThermostat", cg.PollingComponent)

BINDINGS = {
    "round_display": display.Display,
    "thermostat_climate": climate.Climate,
    "display_backlight": light.LightState,
    "room_temperature": number.Number,
    "room_humidity": number.Number,
    "standby_brightness": number.Number,
    "standby_timeout": number.Number,
    "temperature_step": select.Select,
    "humidity_display": select.Select,
    "heating_source": select.Select,
    "window_open": switch.Switch,
    "notification_present": switch.Switch,
    "notification_text": text.Text,
    "notification_acknowledged": binary_sensor.BinarySensor,
    "thermostat_font": font.Font,
    "standby_temperature_font": font.Font,
    "room_temperature_font": font.Font,
    "menu_font": font.Font,
    "notification_font": font.Font,
    "mode_auto_icon": image.Image,
    "mode_heat_icon": image.Image,
    "mode_cool_icon": image.Image,
    "mode_off_icon": image.Image,
    "standby_mode_auto_icon": image.Image,
    "standby_mode_heat_icon": image.Image,
    "standby_mode_cool_icon": image.Image,
    "standby_mode_off_icon": image.Image,
    "room_temperature_icon": image.Image,
    "room_humidity_icon": image.Image,
    "heat_source_heatpump_icon": image.Image,
    "heat_source_radiator_icon": image.Image,
    "heat_source_ac_icon": image.Image,
    "window_open_icon": image.Image,
    "notification_icon": image.Image,
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(RoundThermostat),
    **{cv.Required(key): cv.use_id(type_) for key, type_ in BINDINGS.items()},
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    for key in BINDINGS:
        target = await cg.get_variable(config[key])
        cg.add(getattr(var, f"set_{key}")(target))
