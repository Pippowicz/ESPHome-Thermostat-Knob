#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/display/display.h"
#include "esphome/components/font/font.h"
#include "esphome/components/image/image.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text/text.h"
#include "esphome/core/component.h"
#include "remote_climate_state.h"

namespace esphome::round_thermostat {

class RoundThermostat : public PollingComponent {
public:
  RoundThermostat() : PollingComponent(1000) {}
  ~RoundThermostat();
  void setup() override;
  void start();
  void set_mode(climate::ClimateMode mode);
  void set_source_entity(const std::string &entity) {
    this->source_entity_ = entity;
  }
  void loop() override;
  void update() override;
  void render(display::Display &it);
  void touch(int x, int y);
  void rotate(int direction);
  void set_target_temperature(float value);
  void redraw(bool full = false);
  void notification_changed();
  void standby_brightness_changed(float value);
  void set_round_display(display::Display *value) {
    this->round_display_ = value;
  }
  void set_thermostat_climate(climate::Climate *value) {
    this->thermostat_climate_ = value;
  }
  void set_display_backlight(light::LightState *value) {
    this->display_backlight_ = value;
  }
  void set_room_temperature(number::Number *value) {
    this->room_temperature_ = value;
  }
  void set_room_humidity(number::Number *value) {
    this->room_humidity_ = value;
  }
  void set_standby_brightness(number::Number *value) {
    this->standby_brightness_ = value;
  }
  void set_standby_timeout(number::Number *value) {
    this->standby_timeout_ = value;
  }
  void set_temperature_step(select::Select *value) {
    this->temperature_step_ = value;
  }
  void set_humidity_display(select::Select *value) {
    this->humidity_display_ = value;
  }
  void set_heating_source(select::Select *value) {
    this->heating_source_ = value;
  }
  void set_window_open(switch_::Switch *value) { this->window_open_ = value; }
  void set_notification_present(switch_::Switch *value) {
    this->notification_present_ = value;
  }
  void set_notification_text(text::Text *value) {
    this->notification_text_ = value;
  }
  void set_notification_acknowledged(binary_sensor::BinarySensor *value) {
    this->notification_acknowledged_ = value;
  }
  void set_thermostat_font(font::Font *value) {
    this->thermostat_font_ = value;
  }
  void set_standby_temperature_font(font::Font *value) {
    this->standby_temperature_font_ = value;
  }
  void set_room_temperature_font(font::Font *value) {
    this->room_temperature_font_ = value;
  }
  void set_menu_font(font::Font *value) { this->menu_font_ = value; }
  void set_notification_font(font::Font *value) {
    this->notification_font_ = value;
  }
  void set_mode_auto_icon(image::Image *value) {
    this->mode_auto_icon_ = value;
  }
  void set_mode_heat_icon(image::Image *value) {
    this->mode_heat_icon_ = value;
  }
  void set_mode_cool_icon(image::Image *value) {
    this->mode_cool_icon_ = value;
  }
  void set_mode_off_icon(image::Image *value) { this->mode_off_icon_ = value; }
  void set_standby_mode_auto_icon(image::Image *value) {
    this->standby_mode_auto_icon_ = value;
  }
  void set_standby_mode_heat_icon(image::Image *value) {
    this->standby_mode_heat_icon_ = value;
  }
  void set_standby_mode_cool_icon(image::Image *value) {
    this->standby_mode_cool_icon_ = value;
  }
  void set_standby_mode_off_icon(image::Image *value) {
    this->standby_mode_off_icon_ = value;
  }
  void set_room_temperature_icon(image::Image *value) {
    this->room_temperature_icon_ = value;
  }
  void set_room_humidity_icon(image::Image *value) {
    this->room_humidity_icon_ = value;
  }
  void set_heat_source_heatpump_icon(image::Image *value) {
    this->heat_source_heatpump_icon_ = value;
  }
  void set_heat_source_radiator_icon(image::Image *value) {
    this->heat_source_radiator_icon_ = value;
  }
  void set_heat_source_ac_icon(image::Image *value) {
    this->heat_source_ac_icon_ = value;
  }
  void set_window_open_icon(image::Image *value) {
    this->window_open_icon_ = value;
  }
  void set_notification_icon(image::Image *value) {
    this->notification_icon_ = value;
  }

protected:
  void render_screen_(display::Display &it);
  void ha_loop_();
  void publish_remote_();
  void send_ha_action_(const char *action, const char *key,
                       const std::string &value);
  bool request_ha_target_(float value);
  bool request_ha_mode_(climate::ClimateMode mode);
  float room_temperature_value_() const;
  float room_humidity_value_() const;
  std::string source_entity_;
  RemoteClimateState remote_;
  bool remote_dirty_{false};
  const char *ha_status_() const;

  void wake_();
  void acknowledge_notification_();
  bool started_{false};
  bool redraw_pending_{false};
  uint8_t *border_map_{nullptr};
  uint8_t *arc_map_{nullptr};
  uint8_t *position_map_{nullptr};
  bool lookup_ready_{false};
  bool lookup_failed_{false};
  bool framebuffer_initialized_{false};
  float thermostat_setpoint_{21.0f};
  bool mode_menu_open_{false};
  bool notification_popup_open_{false};
  bool force_full_redraw_{false};
  bool standby_active_{false};
  uint32_t last_activity_ms_{0};
  float normal_backlight_brightness_{1.0f};
  display::Display *round_display_{nullptr};
  climate::Climate *thermostat_climate_{nullptr};
  light::LightState *display_backlight_{nullptr};
  number::Number *room_temperature_{nullptr};
  number::Number *room_humidity_{nullptr};
  number::Number *standby_brightness_{nullptr};
  number::Number *standby_timeout_{nullptr};
  select::Select *temperature_step_{nullptr};
  select::Select *humidity_display_{nullptr};
  select::Select *heating_source_{nullptr};
  switch_::Switch *window_open_{nullptr};
  switch_::Switch *notification_present_{nullptr};
  text::Text *notification_text_{nullptr};
  binary_sensor::BinarySensor *notification_acknowledged_{nullptr};
  font::Font *thermostat_font_{nullptr};
  font::Font *standby_temperature_font_{nullptr};
  font::Font *room_temperature_font_{nullptr};
  font::Font *menu_font_{nullptr};
  font::Font *notification_font_{nullptr};
  image::Image *mode_auto_icon_{nullptr};
  image::Image *mode_heat_icon_{nullptr};
  image::Image *mode_cool_icon_{nullptr};
  image::Image *mode_off_icon_{nullptr};
  image::Image *standby_mode_auto_icon_{nullptr};
  image::Image *standby_mode_heat_icon_{nullptr};
  image::Image *standby_mode_cool_icon_{nullptr};
  image::Image *standby_mode_off_icon_{nullptr};
  image::Image *room_temperature_icon_{nullptr};
  image::Image *room_humidity_icon_{nullptr};
  image::Image *heat_source_heatpump_icon_{nullptr};
  image::Image *heat_source_radiator_icon_{nullptr};
  image::Image *heat_source_ac_icon_{nullptr};
  image::Image *window_open_icon_{nullptr};
  image::Image *notification_icon_{nullptr};
};

} // namespace esphome::round_thermostat
