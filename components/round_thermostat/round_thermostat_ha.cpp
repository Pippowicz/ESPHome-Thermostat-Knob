#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "round_thermostat.h"
#ifdef USE_ROUND_THERMOSTAT_HA
#include "esphome/components/api/api_server.h"
#endif

namespace esphome::round_thermostat {
using Mode = RemoteClimateState::Mode;

static climate::ClimateMode local_mode(Mode mode) {
  switch (mode) {
  case Mode::HEAT:
    return climate::CLIMATE_MODE_HEAT;
  case Mode::COOL:
    return climate::CLIMATE_MODE_COOL;
  case Mode::AUTO:
    return climate::CLIMATE_MODE_AUTO;
  default:
    return climate::CLIMATE_MODE_OFF;
  }
}

void RoundThermostat::setup() {
#ifdef USE_ROUND_THERMOSTAT_HA
  if (this->source_entity_.empty())
    return;
  auto *server = api::global_api_server;
  server->subscribe_home_assistant_state(
      this->source_entity_.c_str(), nullptr, [this](StringRef value) {
        this->remote_.receive_mode(value.str());
        this->remote_dirty_ = true;
      });
  server->subscribe_home_assistant_state(this->source_entity_.c_str(),
                                         "hvac_modes", [this](StringRef value) {
                                           this->remote_.modes = value.str();
                                           this->remote_dirty_ = true;
                                         });
  auto subscribe_number = [this, server](const char *attribute,
                                         std::function<void(float)> callback) {
    server->subscribe_home_assistant_state(
        this->source_entity_.c_str(), attribute,
        [this, callback](StringRef value) {
          const auto parsed = parse_number<float>(value.c_str());
          const float number =
              parsed.has_value() && std::isfinite(*parsed) ? *parsed : NAN;
          callback(number);
          this->remote_dirty_ = true;
        });
  };
  subscribe_number("temperature", [this](float value) {
    this->remote_.receive_target(value);
  });
  subscribe_number("current_temperature",
                   [this](float value) { this->remote_.temperature = value; });
  subscribe_number("current_humidity", [this](float value) {
    this->remote_.humidity = value >= 0 && value <= 100 ? value : NAN;
  });
  subscribe_number("min_temp",
                   [this](float value) { this->remote_.minimum = value; });
  subscribe_number("max_temp",
                   [this](float value) { this->remote_.maximum = value; });
  subscribe_number("target_temp_step", [this](float value) {
    this->remote_.step = std::isfinite(value) && value > 0 ? value : 0.5f;
  });
  ESP_LOGI("round_thermostat", "HA climate source: %s",
           this->source_entity_.c_str());
#endif
}

void RoundThermostat::ha_loop_() {
#ifdef USE_ROUND_THERMOSTAT_HA
  if (this->source_entity_.empty())
    return;
  const bool connected =
      api::global_api_server->is_connected_with_state_subscription();
  if (connected != this->remote_.connected) {
    if (!connected)
      this->remote_.disconnect();
    this->remote_.connected = connected;
    this->remote_dirty_ = true;
  }
  const uint32_t now = millis();
  if (this->remote_.target_due(now)) {
    char value[24];
    snprintf(value, sizeof(value), "%.3f", this->remote_.requested_target);
    this->send_ha_action_("climate.set_temperature", "temperature", value);
    this->remote_.mark_target_sent(now);
  }
  if (this->remote_.expire(now)) {
    ESP_LOGW("round_thermostat",
             "HA command not confirmed; restoring reported state");
    this->remote_dirty_ = true;
  }
  if (this->started_ && this->remote_dirty_) {
    this->remote_dirty_ = false;
    this->publish_remote_();
  }
#endif
}

void RoundThermostat::send_ha_action_(const char *action, const char *key,
                                      const std::string &value) {
#ifdef USE_ROUND_THERMOSTAT_HA
  api::HomeassistantActionRequest request;
  request.service = StringRef(action);
  request.data.init(2);
  api::HomeassistantServiceMap entity;
  entity.key = StringRef("entity_id");
  entity.value = StringRef(this->source_entity_);
  request.data.push_back(entity);
  api::HomeassistantServiceMap data;
  data.key = StringRef(key);
  data.value = StringRef(value);
  request.data.push_back(data);
  api::global_api_server->send_homeassistant_action(request);
#endif
}

bool RoundThermostat::request_ha_target_(float value) {
  if (!this->started_ || !this->remote_.request_target(value, millis()))
    return false;
  this->thermostat_setpoint_ = this->remote_.target();
  this->remote_dirty_ = true;
  return true;
}

bool RoundThermostat::request_ha_mode_(climate::ClimateMode mode) {
  Mode requested = Mode::UNKNOWN;
  switch (mode) {
  case climate::CLIMATE_MODE_OFF:
    requested = Mode::OFF;
    break;
  case climate::CLIMATE_MODE_HEAT:
    requested = Mode::HEAT;
    break;
  case climate::CLIMATE_MODE_COOL:
    requested = Mode::COOL;
    break;
  case climate::CLIMATE_MODE_AUTO:
    requested = Mode::AUTO;
    break;
  default:
    break;
  }
  if (!this->started_ || !this->remote_.request_mode(requested, millis())) {
    ESP_LOGD("round_thermostat", "Mode command ignored: source unavailable, "
                                 "unchanged or mode unsupported");
    return false;
  }
  this->send_ha_action_("climate.set_hvac_mode", "hvac_mode",
                        RemoteClimateState::mode_name(requested));
  this->remote_dirty_ = true;
  return true;
}

void RoundThermostat::publish_remote_() {
  auto *entity = this->thermostat_climate_;
  const bool ready = this->remote_.ready();
  const float target = ready ? this->remote_.target() : NAN;
  const float temperature = this->room_temperature_value_();
  const float humidity = this->room_humidity_value_();
  const auto mode = local_mode(this->remote_.mode());
  const bool changed =
      !RemoteClimateState::equal(entity->target_temperature, target) ||
      !RemoteClimateState::equal(entity->current_temperature, temperature) ||
      !RemoteClimateState::equal(entity->current_humidity, humidity) ||
      entity->mode != mode;
  this->thermostat_setpoint_ = target;
  entity->target_temperature = target;
  entity->current_temperature = temperature;
  entity->current_humidity = humidity;
  entity->mode = mode;
  if (this->remote_.range_valid()) {
    entity->set_visual_min_temperature_override(this->remote_.minimum);
    entity->set_visual_max_temperature_override(this->remote_.maximum);
  }
  // publish_state() does not run the template set_* actions. Never make_call()
  // for incoming HA reports: that would turn a report into another command.
  if (changed)
    entity->publish_state();
  this->redraw(true);
}

float RoundThermostat::room_temperature_value_() const {
  if (this->source_entity_.empty())
    return this->room_temperature_->state;
  return this->remote_.connected && this->remote_.available
             ? this->remote_.temperature
             : NAN;
}

float RoundThermostat::room_humidity_value_() const {
  if (this->source_entity_.empty())
    return this->room_humidity_->state;
  return this->remote_.connected && this->remote_.available
             ? this->remote_.humidity
             : NAN;
}

const char *RoundThermostat::ha_status_() const {
  if (!this->remote_.connected)
    return "HA offline";
  if (!this->remote_.available)
    return "HA wartet";
  if (this->remote_.reported_mode == Mode::UNKNOWN)
    return "HA Modus";
  if (this->remote_.command_failed)
    return "HA Fehler";
  if (!std::isfinite(this->remote_.target()) || !this->remote_.range_valid())
    return "HA Daten";
  return nullptr;
}

} // namespace esphome::round_thermostat
