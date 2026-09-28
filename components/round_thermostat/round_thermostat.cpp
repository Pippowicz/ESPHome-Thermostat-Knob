// Extracted from ESPHome-Thermostat-Knob by Pippowicz.
// License: CC BY-NC 4.0. See README.md for attribution.
#include "round_thermostat.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <esp_heap_caps.h>
#include <string>
#include <vector>

namespace esphome::round_thermostat {
using display::ImageAlign;
using display::TextAlign;

RoundThermostat::~RoundThermostat() {
  heap_caps_free(this->border_map_);
  heap_caps_free(this->arc_map_);
  heap_caps_free(this->position_map_);
}

void RoundThermostat::redraw(bool full) {
  this->force_full_redraw_ |= full;
  this->redraw_pending_ = true;
}

void RoundThermostat::loop() {
  this->ha_loop_();
  // Draw after entity actions have finished publishing their new states.
  if (this->started_ && this->redraw_pending_) {
    this->redraw_pending_ = false;
    this->round_display_->update();
  }
}

void RoundThermostat::start() {
  if (!std::isnan(this->thermostat_climate_->target_temperature)) {
    this->thermostat_setpoint_ = this->thermostat_climate_->target_temperature;
    ESP_LOGI("thermostat", "Wiederhergestellter Sollwert -> %.1f °C",
             this->thermostat_setpoint_);
  }

  this->last_activity_ms_ = millis();
  this->standby_active_ = false;
  this->mode_menu_open_ = false;
  this->notification_popup_open_ = false;
  this->force_full_redraw_ = true;
  if (!this->source_entity_.empty()) {
    this->thermostat_setpoint_ = this->remote_.target();
    this->remote_dirty_ = true;
  }
  this->started_ = true;
  this->redraw(true);
}

void RoundThermostat::wake_() {
  this->standby_active_ = false;
  this->last_activity_ms_ = millis();
  this->mode_menu_open_ = false;
  this->notification_popup_open_ = false;
  auto call = this->display_backlight_->turn_on();
  call.set_brightness(this->normal_backlight_brightness_);
  call.perform();
  this->redraw(true);
}

void RoundThermostat::rotate(int direction) {
  if (!this->started_ || this->notification_popup_open_)
    return;
  if (this->standby_active_) {
    this->wake_();
    return;
  }
  this->last_activity_ms_ = millis();
  const float step =
      this->temperature_step_->current_option() == "1.0 °C" ? 1.0f : 0.5f;
  float value = this->thermostat_setpoint_ + (direction > 0 ? step : -step);
  if (!this->source_entity_.empty()) {
    if (this->request_ha_target_(this->remote_.encoder_target(direction, step)))
      this->redraw();
    return;
  }
  value = std::max(5.0f, std::min(30.0f, value));
  this->thermostat_setpoint_ = value;
  auto call = this->thermostat_climate_->make_call();
  call.set_target_temperature(value);
  call.perform();
}

void RoundThermostat::set_target_temperature(float value) {
  if (!this->source_entity_.empty()) {
    this->request_ha_target_(value);
    // Even rejected commands must restore the template's optimistic local
    // state.
    this->remote_dirty_ = true;
    return;
  }
  this->thermostat_setpoint_ = std::max(5.0f, std::min(30.0f, value));
  this->redraw();
}

void RoundThermostat::set_mode(climate::ClimateMode mode) {
  if (!this->source_entity_.empty()) {
    this->request_ha_mode_(mode);
    this->remote_dirty_ = true;
    return;
  }
  this->redraw(true);
}

void RoundThermostat::acknowledge_notification_() {
  this->notification_acknowledged_->publish_state(true);
  this->set_timeout("notification_ack", 500, [this]() {
    this->notification_acknowledged_->publish_state(false);
  });
}

void RoundThermostat::notification_changed() {
  if (this->notification_popup_open_)
    this->redraw(true);
}

void RoundThermostat::standby_brightness_changed(float value) {
  if (!this->standby_active_)
    return;
  auto call = this->display_backlight_->turn_on();
  call.set_brightness(std::max(0.0f, std::min(1.0f, value / 100.0f)));
  call.perform();
}

void RoundThermostat::touch(int x, int y) {
  if (!this->started_)
    return;

  ESP_LOGI("touch", "Touch x=%d y=%d menu=%s standby=%s notification=%s", x, y,
           this->mode_menu_open_ ? "open" : "closed",
           this->standby_active_ ? "yes" : "no",
           this->notification_popup_open_ ? "open" : "closed");

  if (this->notification_popup_open_) {

    ESP_LOGI("thermostat", "Benachrichtigung bestaetigt");

    acknowledge_notification_();

    this->notification_popup_open_ = false;
    this->mode_menu_open_ = false;
    this->last_activity_ms_ = millis();
    this->force_full_redraw_ = true;

    this->redraw();
    return;
  }

  if (this->standby_active_) {
    this->wake_();
    return;
  }

  this->last_activity_ms_ = millis();

  if (!this->mode_menu_open_) {

    this->mode_menu_open_ = true;

    ESP_LOGI("thermostat", "Radiales Modus-Menue geoeffnet");

    this->redraw();
    return;
  }

  const int CX = 120;
  const int CY = 120;

  const int dx = x - CX;
  const int dy = y - CY;

  const int d2 = dx * dx + dy * dy;

  const int OUTER_R = 106;
  // Deliberately larger than the drawn 32px inner radius.
  const int INNER_R = 42;

  const int OUTER_R2 = OUTER_R * OUTER_R;

  const int INNER_R2 = INNER_R * INNER_R;

  if (d2 > OUTER_R2 || d2 < INNER_R2) {

    this->mode_menu_open_ = false;
    this->force_full_redraw_ = true;
    this->redraw();
    return;
  }

  float angle = atan2f((float)dy, (float)dx) * 180.0f / 3.14159265359f;

  if (angle < 0.0f)
    angle += 360.0f;

  int segment;

  if (angle >= 54.0f && angle < 126.0f) {
    segment = 4;
  }

  else if (angle >= 126.0f && angle < 198.0f) {
    segment = 3;
  }

  else if (angle >= 198.0f && angle < 270.0f) {
    segment = 1;
  }

  else if (angle >= 270.0f && angle < 342.0f) {
    segment = 0;
  }

  else {
    segment = 2;
  }

  if (segment == 4) {

    if (!this->notification_present_->state) {
      ESP_LOGI("thermostat", "Benachrichtigungssegment inaktiv");
      return;
    }

    this->mode_menu_open_ = false;
    this->notification_popup_open_ = true;
    this->force_full_redraw_ = true;

    ESP_LOGI("thermostat", "Benachrichtigung geoeffnet");

    this->redraw();
    return;
  }

  auto call = this->thermostat_climate_->make_call();

  if (segment == 0) {

    call.set_mode(climate::CLIMATE_MODE_AUTO);

    ESP_LOGI("thermostat", "Touch -> AUTO");
  }

  else if (segment == 1) {

    call.set_mode(climate::CLIMATE_MODE_HEAT);

    ESP_LOGI("thermostat", "Touch -> HEAT");
  }

  else if (segment == 2) {

    call.set_mode(climate::CLIMATE_MODE_COOL);

    ESP_LOGI("thermostat", "Touch -> COOL");
  }

  else {

    call.set_mode(climate::CLIMATE_MODE_OFF);

    ESP_LOGI("thermostat", "Touch -> OFF");
  }

  this->mode_menu_open_ = false;
  this->force_full_redraw_ = true;

  call.perform();
}

void RoundThermostat::update() {
  if (!this->started_)
    return;
  if (this->standby_active_ || this->notification_popup_open_) {
    return;
  }

  const float timeout_seconds = this->standby_timeout_->state;

  if (std::isnan(timeout_seconds) || timeout_seconds <= 0.0f) {
    return;
  }

  const uint32_t timeout_ms = (uint32_t)(timeout_seconds * 1000.0f);

  const uint32_t now = millis();

  if ((uint32_t)(now - this->last_activity_ms_) < timeout_ms) {
    return;
  }

  float current_brightness =
      this->display_backlight_->remote_values.get_brightness();

  if (current_brightness > 0.0f) {
    this->normal_backlight_brightness_ = current_brightness;
  }

  this->standby_active_ = true;
  this->mode_menu_open_ = false;
  this->notification_popup_open_ = false;
  this->force_full_redraw_ = true;

  float standby_level = this->standby_brightness_->state / 100.0f;

  if (standby_level < 0.0f)
    standby_level = 0.0f;

  if (standby_level > 1.0f)
    standby_level = 1.0f;

  auto light_call = this->display_backlight_->turn_on();

  light_call.set_brightness(standby_level);

  light_call.perform();

  ESP_LOGI("thermostat", "Standby aktiviert: %.0f%%", standby_level * 100.0f);

  this->redraw();
}

void RoundThermostat::render(display::Display &it) {
  this->render_screen_(it);
  if (this->started_ && !this->source_entity_.empty() &&
      !this->notification_popup_open_) {
    const char *status = this->ha_status_();
    if (status != nullptr)
      it.print(120, 224, this->menu_font_, Color(230, 160, 60),
               TextAlign::CENTER, status);
  }
}

void RoundThermostat::render_screen_(display::Display &it) {
  if (!this->started_)
    return;
  const float MIN_TEMP =
      !this->source_entity_.empty() && this->remote_.range_valid()
          ? this->remote_.minimum
          : 5.0f;
  const float MAX_TEMP =
      !this->source_entity_.empty() && this->remote_.range_valid()
          ? this->remote_.maximum
          : 30.0f;
  const float SET_TEMP = this->thermostat_setpoint_;

  const int WIDTH = 240;
  const int HEIGHT = 240;
  const int PIXELS = WIDTH * HEIGHT;

  const int CX = 120;
  const int CY = 120;

  const float ARC_RADIUS = 108.0f;
  const float ARC_WIDTH = 14.0f;
  const float BORDER_WIDTH = 18.0f;

  const float START_DEG = 135.0f;
  const float SWEEP_DEG = 270.0f;
  const float PI_F = 3.14159265359f;

  const Color BG(5, 5, 5);

  const float BORDER_R = 180.0f;
  const float BORDER_G = 180.0f;
  const float BORDER_B = 180.0f;

  const float EMPTY_R = 45.0f;
  const float EMPTY_G = 45.0f;
  const float EMPTY_B = 45.0f;

  const Color MODE_OFF(170, 170, 170);
  const Color MODE_HEAT(255, 70, 45);
  const Color MODE_COOL(65, 155, 255);
  const Color MODE_AUTO(65, 200, 100);

  // ==========================================================
  // RING-LOOKUP
  // ==========================================================

  if (!this->lookup_ready_ && !this->lookup_failed_) {

    ESP_LOGI("thermostat", "Erzeuge 4x4 Ring-Lookup im PSRAM...");

    this->border_map_ = (uint8_t *)heap_caps_malloc(
        PIXELS, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    this->arc_map_ = (uint8_t *)heap_caps_malloc(PIXELS, MALLOC_CAP_SPIRAM |
                                                             MALLOC_CAP_8BIT);

    this->position_map_ = (uint8_t *)heap_caps_malloc(
        PIXELS, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (this->border_map_ == nullptr || this->arc_map_ == nullptr ||
        this->position_map_ == nullptr) {

      ESP_LOGE("thermostat",
               "PSRAM-Allokation fuer Ring-Lookup fehlgeschlagen");

      if (this->border_map_ != nullptr) {
        heap_caps_free(this->border_map_);
        this->border_map_ = nullptr;
      }

      if (this->arc_map_ != nullptr) {
        heap_caps_free(this->arc_map_);
        this->arc_map_ = nullptr;
      }

      if (this->position_map_ != nullptr) {
        heap_caps_free(this->position_map_);
        this->position_map_ = nullptr;
      }

      this->lookup_failed_ = true;

    } else {

      memset(this->border_map_, 0, PIXELS);
      memset(this->arc_map_, 0, PIXELS);
      memset(this->position_map_, 0, PIXELS);

      const int SS = 4;
      const float INV_SS = 1.0f / SS;
      const float SAMPLE_COUNT = SS * SS;

      const float border_half = BORDER_WIDTH * 0.5f;

      const float arc_half = ARC_WIDTH * 0.5f;

      const float border_inner = ARC_RADIUS - border_half;

      const float border_outer = ARC_RADIUS + border_half;

      const float arc_inner = ARC_RADIUS - arc_half;

      const float arc_outer = ARC_RADIUS + arc_half;

      const float border_inner2 = border_inner * border_inner;

      const float border_outer2 = border_outer * border_outer;

      const float arc_inner2 = arc_inner * arc_inner;

      const float arc_outer2 = arc_outer * arc_outer;

      const float start_rad = START_DEG * PI_F / 180.0f;

      const float end_rad = (START_DEG + SWEEP_DEG) * PI_F / 180.0f;

      const float start_x = CX + cosf(start_rad) * ARC_RADIUS;

      const float start_y = CY + sinf(start_rad) * ARC_RADIUS;

      const float end_x = CX + cosf(end_rad) * ARC_RADIUS;

      const float end_y = CY + sinf(end_rad) * ARC_RADIUS;

      const float border_cap2 = border_half * border_half;

      const float arc_cap2 = arc_half * arc_half;

      const float reject_outer = ARC_RADIUS + border_half + 1.5f;

      const float reject_inner = ARC_RADIUS - border_half - 1.5f;

      const float reject_outer2 = reject_outer * reject_outer;

      const float reject_inner2 = reject_inner * reject_inner;

      for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {

          float center_dx = (x + 0.5f) - CX;

          float center_dy = (y + 0.5f) - CY;

          float center_d2 = center_dx * center_dx + center_dy * center_dy;

          if (center_d2 > reject_outer2 || center_d2 < reject_inner2) {
            continue;
          }

          int border_count = 0;
          int arc_count = 0;
          float position_sum = 0.0f;

          for (int sy = 0; sy < SS; sy++) {
            for (int sx = 0; sx < SS; sx++) {

              float sample_x = x + (sx + 0.5f) * INV_SS;

              float sample_y = y + (sy + 0.5f) * INV_SS;

              float dx = sample_x - CX;

              float dy = sample_y - CY;

              float d2 = dx * dx + dy * dy;

              float angle = atan2f(dy, dx) * 180.0f / PI_F;

              if (angle < 0.0f)
                angle += 360.0f;

              float relative = angle - START_DEG;

              if (relative < 0.0f)
                relative += 360.0f;

              bool angular_inside = relative <= SWEEP_DEG;

              float position = relative / SWEEP_DEG;

              float sdx = sample_x - start_x;

              float sdy = sample_y - start_y;

              float edx = sample_x - end_x;

              float edy = sample_y - end_y;

              float start_d2 = sdx * sdx + sdy * sdy;

              float end_d2 = edx * edx + edy * edy;

              bool border_inside = false;
              bool arc_inside = false;

              if (angular_inside && d2 >= border_inner2 &&
                  d2 <= border_outer2) {
                border_inside = true;
              }

              if (start_d2 <= border_cap2 || end_d2 <= border_cap2) {
                border_inside = true;
              }

              if (angular_inside && d2 >= arc_inner2 && d2 <= arc_outer2) {
                arc_inside = true;
              }

              if (start_d2 <= arc_cap2) {
                arc_inside = true;
                position = 0.0f;
              }

              if (end_d2 <= arc_cap2) {
                arc_inside = true;
                position = 1.0f;
              }

              if (border_inside)
                border_count++;

              if (arc_inside) {
                arc_count++;
                position_sum += position;
              }
            }
          }

          const int index = y * WIDTH + x;

          if (border_count > 0) {
            this->border_map_[index] =
                (uint8_t)roundf(255.0f * border_count / SAMPLE_COUNT);
          }

          if (arc_count > 0) {
            this->arc_map_[index] =
                (uint8_t)roundf(255.0f * arc_count / SAMPLE_COUNT);

            float average_position = position_sum / arc_count;

            if (average_position < 0.0f)
              average_position = 0.0f;

            if (average_position > 1.0f)
              average_position = 1.0f;

            this->position_map_[index] =
                (uint8_t)roundf(average_position * 255.0f);
          }
        }
      }

      this->lookup_ready_ = true;

      ESP_LOGI("thermostat", "4x4 Ring-Lookup fertig");
    }
  }

  // ==========================================================
  // FRAMEBUFFER
  // ==========================================================

  if (!this->framebuffer_initialized_ || this->force_full_redraw_) {
    it.fill(BG);
    this->framebuffer_initialized_ = true;
    this->force_full_redraw_ = false;
  }

  // ==========================================================
  // BENACHRICHTIGUNGS-POPUP
  // ==========================================================

  if (this->notification_popup_open_) {

    const Color POPUP_BG(5, 5, 5);
    const Color POPUP_COLOR(210, 210, 210);
    const Color POPUP_ICON_COLOR(190, 190, 190);

    it.fill(POPUP_BG);

    it.image(120, 38, this->notification_icon_, ImageAlign::CENTER,
             POPUP_ICON_COLOR, POPUP_BG);

    std::string message = this->notification_text_->state;

    if (message.empty()) {
      message = "Keine Benachrichtigung";
    }

    std::vector<std::string> lines;
    std::string line;
    std::string word;

    auto push_word = [&]() {
      if (word.empty())
        return;

      if (line.empty()) {
        line = word;
      } else if ((line.size() + 1 + word.size()) <= 21) {
        line += " ";
        line += word;
      } else {
        lines.push_back(line);
        line = word;
      }

      word.clear();
    };

    for (size_t i = 0; i < message.size(); i++) {
      const char c = message[i];

      if (c == '\n') {
        push_word();

        if (!line.empty()) {
          lines.push_back(line);
          line.clear();
        }

        continue;
      }

      if (c == ' ') {
        push_word();
        continue;
      }

      word += c;
    }

    push_word();

    if (!line.empty()) {
      lines.push_back(line);
    }

    const int MAX_LINES = 6;

    if ((int)lines.size() > MAX_LINES) {
      lines.resize(MAX_LINES);

      if (lines[MAX_LINES - 1].size() > 17) {
        lines[MAX_LINES - 1].resize(17);
      }

      lines[MAX_LINES - 1] += "...";
    }

    const int LINE_HEIGHT = 23;

    const int total_height = lines.size() * LINE_HEIGHT;

    int text_y = 125 - total_height / 2;

    if (text_y < 68)
      text_y = 68;

    for (size_t i = 0; i < lines.size(); i++) {
      it.print(120, text_y + i * LINE_HEIGHT, this->notification_font_,
               POPUP_COLOR, TextAlign::TOP_CENTER, lines[i].c_str());
    }

    return;
  }

  // ==========================================================
  // STANDBY
  // ==========================================================

  if (this->standby_active_) {

    const Color STANDBY_BG(0, 0, 0);
    const Color STANDBY_COLOR(150, 150, 150);

    it.fill(STANDBY_BG);

    const auto standby_mode = this->thermostat_climate_->mode;

    if (standby_mode == climate::CLIMATE_MODE_OFF) {
      it.image(120, 55, this->standby_mode_off_icon_, ImageAlign::CENTER,
               STANDBY_COLOR, STANDBY_BG);
    }

    else if (standby_mode == climate::CLIMATE_MODE_HEAT) {
      it.image(120, 55, this->standby_mode_heat_icon_, ImageAlign::CENTER,
               STANDBY_COLOR, STANDBY_BG);
    }

    else if (standby_mode == climate::CLIMATE_MODE_COOL) {
      it.image(120, 55, this->standby_mode_cool_icon_, ImageAlign::CENTER,
               STANDBY_COLOR, STANDBY_BG);
    }

    else if (standby_mode == climate::CLIMATE_MODE_AUTO) {
      it.image(120, 55, this->standby_mode_auto_icon_, ImageAlign::CENTER,
               STANDBY_COLOR, STANDBY_BG);
    }

    if (standby_mode == climate::CLIMATE_MODE_OFF &&
        (this->source_entity_.empty() || this->remote_.ready())) {
      it.print(120, 125, this->standby_temperature_font_, STANDBY_COLOR,
               TextAlign::CENTER, "Off");
    } else {
      it.printf(120, 125, this->standby_temperature_font_, STANDBY_COLOR,
                TextAlign::CENTER,
                std::isfinite(this->room_temperature_value_()) ? "%.1f °C"
                                                               : "-- °C",
                this->room_temperature_value_());
    }

    if (this->notification_present_->state) {
      it.image(120, 198, this->notification_icon_, ImageAlign::CENTER,
               STANDBY_COLOR, STANDBY_BG);
    }

    return;
  }

  // ==========================================================
  // RADIALES MODUS-MENÜ - 5 x 72°
  // ==========================================================

  if (this->mode_menu_open_) {

    const int MENU_OUTER_R = 106;
    const int MENU_INNER_R = 32;

    const int OUTER_R2 = MENU_OUTER_R * MENU_OUTER_R;

    const int INNER_R2 = MENU_INNER_R * MENU_INNER_R;

    const Color MENU_BG(5, 5, 5);
    const Color MENU_NORMAL(28, 31, 35);
    const Color MENU_BORDER(68, 73, 80);

    const Color AUTO_BG(18, 61, 45);
    const Color HEAT_BG(92, 37, 24);
    const Color COOL_BG(22, 58, 91);
    const Color OFF_BG(55, 57, 62);

    const Color NOTIFICATION_BG(70, 55, 20);

    const Color TEXT_NORMAL(225, 225, 230);

    const Color NOTIFICATION_COLOR(230, 190, 70);

    const auto mode = this->thermostat_climate_->mode;

    it.fill(MENU_BG);

    for (int y = CY - MENU_OUTER_R; y <= CY + MENU_OUTER_R; y++) {
      for (int x = CX - MENU_OUTER_R; x <= CX + MENU_OUTER_R; x++) {

        const int dx = x - CX;
        const int dy = y - CY;

        const int d2 = dx * dx + dy * dy;

        if (d2 > OUTER_R2 || d2 < INNER_R2) {
          continue;
        }

        float angle = atan2f((float)dy, (float)dx) * 180.0f / PI_F;

        if (angle < 0.0f)
          angle += 360.0f;

        int segment;

        if (angle >= 54.0f && angle < 126.0f) {
          segment = 4;
        }

        else if (angle >= 126.0f && angle < 198.0f) {
          segment = 3;
        }

        else if (angle >= 198.0f && angle < 270.0f) {
          segment = 1;
        }

        else if (angle >= 270.0f && angle < 342.0f) {
          segment = 0;
        }

        else {
          segment = 2;
        }

        Color fill = MENU_NORMAL;

        if (segment == 0 && mode == climate::CLIMATE_MODE_AUTO) {
          fill = AUTO_BG;
        }

        else if (segment == 1 && mode == climate::CLIMATE_MODE_HEAT) {
          fill = HEAT_BG;
        }

        else if (segment == 2 && mode == climate::CLIMATE_MODE_COOL) {
          fill = COOL_BG;
        }

        else if (segment == 3 && mode == climate::CLIMATE_MODE_OFF) {
          fill = OFF_BG;
        }

        else if (segment == 4 && this->notification_present_->state) {
          fill = NOTIFICATION_BG;
        }

        it.draw_pixel_at(x, y, fill);
      }
    }

    const float separator_angles[5] = {54.0f, 126.0f, 198.0f, 270.0f, 342.0f};

    for (int i = 0; i < 5; i++) {

      const float a = separator_angles[i] * PI_F / 180.0f;

      const int x1 = CX + cosf(a) * MENU_INNER_R;

      const int y1 = CY + sinf(a) * MENU_INNER_R;

      const int x2 = CX + cosf(a) * MENU_OUTER_R;

      const int y2 = CY + sinf(a) * MENU_OUTER_R;

      it.line(x1, y1, x2, y2, MENU_BORDER);
    }

    it.circle(CX, CY, MENU_OUTER_R, MENU_BORDER);

    it.circle(CX, CY, MENU_OUTER_R - 1, MENU_BORDER);

    it.filled_circle(CX, CY, MENU_INNER_R, MENU_BG);

    it.circle(CX, CY, MENU_INNER_R, MENU_BORDER);

    it.circle(CX, CY, MENU_INNER_R - 1, MENU_BORDER);

    // AUTO
    it.image(164, 59, this->mode_auto_icon_, ImageAlign::CENTER, MODE_AUTO,
             mode == climate::CLIMATE_MODE_AUTO ? AUTO_BG : MENU_NORMAL);

    it.print(
        164, 82, this->menu_font_,
        (!this->source_entity_.empty() &&
         !this->remote_.supports(RemoteClimateState::Mode::AUTO))
            ? Color(85, 85, 85)
            : (mode == climate::CLIMATE_MODE_AUTO ? MODE_AUTO : TEXT_NORMAL),
        TextAlign::CENTER, "Auto");

    // HEIZEN
    it.image(76, 59, this->mode_heat_icon_, ImageAlign::CENTER, MODE_HEAT,
             mode == climate::CLIMATE_MODE_HEAT ? HEAT_BG : MENU_NORMAL);

    it.print(
        76, 82, this->menu_font_,
        (!this->source_entity_.empty() &&
         !this->remote_.supports(RemoteClimateState::Mode::HEAT))
            ? Color(85, 85, 85)
            : (mode == climate::CLIMATE_MODE_HEAT ? MODE_HEAT : TEXT_NORMAL),
        TextAlign::CENTER, "Heizen");

    // KÜHLEN
    it.image(191, 143, this->mode_cool_icon_, ImageAlign::CENTER, MODE_COOL,
             mode == climate::CLIMATE_MODE_COOL ? COOL_BG : MENU_NORMAL);

    it.print(
        184, 166, this->menu_font_,
        (!this->source_entity_.empty() &&
         !this->remote_.supports(RemoteClimateState::Mode::COOL))
            ? Color(85, 85, 85)
            : (mode == climate::CLIMATE_MODE_COOL ? MODE_COOL : TEXT_NORMAL),
        TextAlign::CENTER, "Kühlen");

    // AUS
    it.image(49, 143, this->mode_off_icon_, ImageAlign::CENTER, MODE_OFF,
             mode == climate::CLIMATE_MODE_OFF ? OFF_BG : MENU_NORMAL);

    it.print(54, 166, this->menu_font_,
             (!this->source_entity_.empty() &&
              !this->remote_.supports(RemoteClimateState::Mode::OFF))
                 ? Color(85, 85, 85)
                 : (mode == climate::CLIMATE_MODE_OFF ? MODE_OFF : TEXT_NORMAL),
             TextAlign::CENTER, "Aus");

    // BENACHRICHTIGUNG
    if (this->notification_present_->state) {
      it.image(120, 195, this->notification_icon_, ImageAlign::CENTER,
               NOTIFICATION_COLOR, NOTIFICATION_BG);
    }

    return;
  }

  // ==========================================================
  // NORMALER THERMOSTAT-SCREEN
  // ==========================================================

  float normalized =
      (std::isfinite(SET_TEMP) ? (SET_TEMP - MIN_TEMP) / (MAX_TEMP - MIN_TEMP)
                               : 0.0f);

  if (normalized < 0.0f)
    normalized = 0.0f;

  if (normalized > 1.0f)
    normalized = 1.0f;

  if (this->lookup_ready_) {

    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {

        const int index = y * WIDTH + x;

        const uint8_t border_value = this->border_map_[index];

        if (border_value == 0)
          continue;

        const float border_cov = border_value / 255.0f;

        float r = 5.0f + (BORDER_R - 5.0f) * border_cov;

        float g = 5.0f + (BORDER_G - 5.0f) * border_cov;

        float b = 5.0f + (BORDER_B - 5.0f) * border_cov;

        const uint8_t arc_value = this->arc_map_[index];

        if (arc_value > 0) {

          const float arc_cov = arc_value / 255.0f;

          const float position = this->position_map_[index] / 255.0f;

          float arc_r;
          float arc_g;
          float arc_b;

          if (position <= normalized) {

            arc_r = 255.0f * position;

            arc_g = 0.0f;

            arc_b = 255.0f * (1.0f - position);

          } else {

            arc_r = EMPTY_R;
            arc_g = EMPTY_G;
            arc_b = EMPTY_B;
          }

          r = r * (1.0f - arc_cov) + arc_r * arc_cov;

          g = g * (1.0f - arc_cov) + arc_g * arc_cov;

          b = b * (1.0f - arc_cov) + arc_b * arc_cov;
        }

        it.draw_pixel_at(x, y, Color((uint8_t)r, (uint8_t)g, (uint8_t)b));
      }
    }
  }

  // ==========================================================
  // MARKER
  // ==========================================================

  float marker_deg = START_DEG + normalized * SWEEP_DEG;

  float marker_rad = marker_deg * PI_F / 180.0f;

  float marker_cx = CX + cosf(marker_rad) * ARC_RADIUS;

  float marker_cy = CY + sinf(marker_rad) * ARC_RADIUS;

  const float MARKER_OUTER = 8.0f;
  const float MARKER_INNER = 4.5f;
  const int MARKER_SS = 4;

  const float MARKER_INV_SS = 1.0f / MARKER_SS;

  const float MARKER_SAMPLE_COUNT = MARKER_SS * MARKER_SS;

  const float MARKER_OUTER2 = MARKER_OUTER * MARKER_OUTER;

  const float MARKER_INNER2 = MARKER_INNER * MARKER_INNER;

  int mx0 = (int)floorf(marker_cx - MARKER_OUTER - 1);

  int mx1 = (int)ceilf(marker_cx + MARKER_OUTER + 1);

  int my0 = (int)floorf(marker_cy - MARKER_OUTER - 1);

  int my1 = (int)ceilf(marker_cy + MARKER_OUTER + 1);

  for (int y = my0; y <= my1; y++) {
    for (int x = mx0; x <= mx1; x++) {

      if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
        continue;
      }

      int outer_count = 0;
      int inner_count = 0;

      for (int sy = 0; sy < MARKER_SS; sy++) {
        for (int sx = 0; sx < MARKER_SS; sx++) {

          float sample_x = x + (sx + 0.5f) * MARKER_INV_SS;

          float sample_y = y + (sy + 0.5f) * MARKER_INV_SS;

          float dx = sample_x - marker_cx;

          float dy = sample_y - marker_cy;

          float dist2 = dx * dx + dy * dy;

          if (dist2 <= MARKER_OUTER2)
            outer_count++;

          if (dist2 <= MARKER_INNER2)
            inner_count++;
        }
      }

      if (outer_count == 0)
        continue;

      const float p = normalized;

      const float base_r = 255.0f * p;

      const float base_g = 0.0f;

      const float base_b = 255.0f * (1.0f - p);

      const float outer_cov = outer_count / MARKER_SAMPLE_COUNT;

      const float inner_cov = inner_count / MARKER_SAMPLE_COUNT;

      float r = base_r * (1.0f - outer_cov) + 10.0f * outer_cov;

      float g = base_g * (1.0f - outer_cov) + 10.0f * outer_cov;

      float b = base_b * (1.0f - outer_cov) + 10.0f * outer_cov;

      r = r * (1.0f - inner_cov) + 255.0f * inner_cov;

      g = g * (1.0f - inner_cov) + 255.0f * inner_cov;

      b = b * (1.0f - inner_cov) + 255.0f * inner_cov;

      it.draw_pixel_at(x, y, Color((uint8_t)r, (uint8_t)g, (uint8_t)b));
    }
  }

  // ==========================================================
  // MODUS-STATUS
  // ==========================================================

  it.filled_rectangle(104, 41, 32, 40, BG);

  const auto climate_mode = this->thermostat_climate_->mode;

  if (climate_mode == climate::CLIMATE_MODE_OFF) {
    it.image(120, 57, this->mode_off_icon_, ImageAlign::CENTER, MODE_OFF, BG);
  }

  else if (climate_mode == climate::CLIMATE_MODE_HEAT) {
    it.image(120, 57, this->mode_heat_icon_, ImageAlign::CENTER, MODE_HEAT, BG);
  }

  else if (climate_mode == climate::CLIMATE_MODE_COOL) {
    it.image(120, 57, this->mode_cool_icon_, ImageAlign::CENTER, MODE_COOL, BG);
  }

  else if (climate_mode == climate::CLIMATE_MODE_AUTO) {
    it.image(120, 57, this->mode_auto_icon_, ImageAlign::CENTER, MODE_AUTO, BG);
  }

  // ==========================================================
  // SOLLTEMPERATUR / OFF
  // ==========================================================

  it.filled_rectangle(50, 84, 140, 52, BG);

  if (climate_mode == climate::CLIMATE_MODE_OFF &&
      (this->source_entity_.empty() || this->remote_.ready())) {
    it.print(CX, CY - 15, this->thermostat_font_, Color(255, 255, 255),
             TextAlign::CENTER, "Off");
  } else {
    it.printf(CX, CY - 15, this->thermostat_font_, Color(255, 255, 255),
              TextAlign::CENTER, std::isfinite(SET_TEMP) ? "%.1f °C" : "-- °C",
              SET_TEMP);
  }

  // ==========================================================
  // ISTTEMPERATUR + LUFTFEUCHTIGKEIT
  // ==========================================================

  const float ROOM_TEMP = this->room_temperature_value_();

  const bool SHOW_HUMIDITY = this->humidity_display_->current_option() == "An";

  const Color ROOM_COLOR(190, 190, 190);

  for (int y = 137; y <= 182; y++) {
    for (int x = 24; x <= 216; x++) {

      const int index = y * WIDTH + x;

      if (!this->lookup_ready_ || this->border_map_[index] == 0) {
        it.draw_pixel_at(x, y, BG);
      }
    }
  }

  if (SHOW_HUMIDITY) {

    it.image(49, 153, this->room_temperature_icon_, ImageAlign::CENTER,
             ROOM_COLOR, BG);

    it.printf(65, 151, this->room_temperature_font_, ROOM_COLOR,
              TextAlign::CENTER_LEFT,
              std::isfinite(ROOM_TEMP) ? "%.1f °C" : "-- °C", ROOM_TEMP);

    const float ROOM_HUMIDITY = this->room_humidity_value_();

    it.image(142, 153, this->room_humidity_icon_, ImageAlign::CENTER,
             ROOM_COLOR, BG);

    it.printf(158, 151, this->room_temperature_font_, ROOM_COLOR,
              TextAlign::CENTER_LEFT,
              std::isfinite(ROOM_HUMIDITY) ? "%.0f%%" : "--%%", ROOM_HUMIDITY);

  } else {

    it.image(83, 153, this->room_temperature_icon_, ImageAlign::CENTER,
             ROOM_COLOR, BG);

    it.printf(100, 151, this->room_temperature_font_, ROOM_COLOR,
              TextAlign::CENTER_LEFT,
              std::isfinite(ROOM_TEMP) ? "%.1f °C" : "-- °C", ROOM_TEMP);
  }

  // ==========================================================
  // HEIZQUELLE + FENSTER + BENACHRICHTIGUNG
  // ==========================================================

  const bool SHOW_HEAT_SOURCE = climate_mode == climate::CLIMATE_MODE_HEAT ||
                                climate_mode == climate::CLIMATE_MODE_AUTO;

  const bool WINDOW_OPEN = this->window_open_->state;

  const bool NOTIFICATION_PRESENT = this->notification_present_->state;

  const Color STATUS_COLOR(190, 190, 190);

  it.filled_rectangle(64, 184, 112, 30, BG);

  int status_count = 0;

  if (SHOW_HEAT_SOURCE)
    status_count++;

  if (WINDOW_OPEN)
    status_count++;

  if (NOTIFICATION_PRESENT)
    status_count++;

  int status_index = 0;

  auto get_status_x = [&](int index) -> int {
    if (status_count <= 1)
      return 120;

    if (status_count == 2)
      return index == 0 ? 101 : 139;

    if (index == 0)
      return 82;

    if (index == 1)
      return 120;

    return 158;
  };

  const int STATUS_Y = 199;

  if (SHOW_HEAT_SOURCE) {

    const int icon_x = get_status_x(status_index++);

    if (this->heating_source_->current_option() == "Wärmepumpe") {
      it.image(icon_x, STATUS_Y, this->heat_source_heatpump_icon_,
               ImageAlign::CENTER, STATUS_COLOR, BG);
    }

    else if (this->heating_source_->current_option() == "Heizkörper") {
      it.image(icon_x, STATUS_Y, this->heat_source_radiator_icon_,
               ImageAlign::CENTER, STATUS_COLOR, BG);
    }

    else {
      it.image(icon_x, STATUS_Y, this->heat_source_ac_icon_, ImageAlign::CENTER,
               STATUS_COLOR, BG);
    }
  }

  if (WINDOW_OPEN) {

    const int icon_x = get_status_x(status_index++);

    it.image(icon_x, STATUS_Y, this->window_open_icon_, ImageAlign::CENTER,
             STATUS_COLOR, BG);
  }

  if (NOTIFICATION_PRESENT) {

    const int icon_x = get_status_x(status_index++);

    it.image(icon_x, STATUS_Y, this->notification_icon_, ImageAlign::CENTER,
             STATUS_COLOR, BG);
  }
}

} // namespace esphome::round_thermostat
