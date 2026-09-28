#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace esphome::round_thermostat {

// Transport-independent state: HA reports never become outbound commands.
struct RemoteClimateState {
  enum class Mode { UNKNOWN, OFF, HEAT, COOL, AUTO };
  bool connected{false};
  bool available{false};
  Mode reported_mode{Mode::UNKNOWN};
  float reported_target{NAN};
  float temperature{NAN};
  float humidity{NAN};
  float minimum{NAN};
  float maximum{NAN};
  float step{0.5f};
  std::string modes;
  bool target_pending{false};
  bool target_sent{false};
  float requested_target{NAN};
  uint32_t target_at{0};
  bool mode_pending{false};
  Mode requested_mode{Mode::UNKNOWN};
  uint32_t mode_at{0};
  bool command_failed{false};

  static bool equal(float a, float b) {
    return (std::isnan(a) && std::isnan(b)) || std::fabs(a - b) < 0.01f;
  }
  static Mode parse_mode(const std::string &s) {
    if (s == "off")
      return Mode::OFF;
    if (s == "heat")
      return Mode::HEAT;
    if (s == "cool")
      return Mode::COOL;
    if (s == "auto")
      return Mode::AUTO;
    return Mode::UNKNOWN; // heat_cool is not auto: it has two setpoints.
  }
  static const char *mode_name(Mode m) {
    switch (m) {
    case Mode::OFF:
      return "off";
    case Mode::HEAT:
      return "heat";
    case Mode::COOL:
      return "cool";
    case Mode::AUTO:
      return "auto";
    default:
      return "unknown";
    }
  }
  bool supports(Mode m) const {
    if (m == Mode::UNKNOWN)
      return false;
    // HA attribute lists may use JSON or Python string representation.
    const std::string token = mode_name(m);
    return modes.find("'" + token + "'") != std::string::npos ||
           modes.find("\"" + token + "\"") != std::string::npos;
  }
  bool ready() const {
    return connected && available && reported_mode != Mode::UNKNOWN;
  }
  bool range_valid() const {
    return std::isfinite(minimum) && std::isfinite(maximum) &&
           minimum < maximum;
  }
  float target() const {
    return target_pending ? requested_target : reported_target;
  }
  Mode mode() const { return mode_pending ? requested_mode : reported_mode; }
  void disconnect() { *this = RemoteClimateState{}; }
  void receive_mode(const std::string &s) {
    available = !s.empty() && s != "unknown" && s != "unavailable";
    reported_mode = parse_mode(s);
    if (reported_mode == Mode::UNKNOWN)
      target_pending = false;
    if (!available) {
      target_pending = mode_pending = false;
      reported_target = temperature = humidity = NAN;
    }
    if (mode_pending && reported_mode == requested_mode) {
      mode_pending = false;
      command_failed = false;
    }
  }
  void receive_target(float value) {
    reported_target = value;
    if (target_pending && equal(value, requested_target)) {
      target_pending = false;
      command_failed = false;
    }
  }
  float encoder_target(int direction, float preferred_step) const {
    const float increment = std::max(preferred_step, step);
    return target() + (direction > 0 ? increment : -increment);
  }
  bool request_target(float value, uint32_t now) {
    if (!ready() || !range_valid() || !std::isfinite(reported_target) ||
        !std::isfinite(value))
      return false;
    value = std::round(value / step) * step;
    value = std::max(minimum, std::min(maximum, value));
    if (equal(value, target()))
      return false;
    requested_target = value;
    target_pending = true;
    target_sent = false;
    target_at = now;
    command_failed = false;
    return true;
  }
  bool request_mode(Mode value, uint32_t now) {
    if (!connected || !available || !supports(value) || value == mode())
      return false;
    requested_mode = value;
    mode_pending = true;
    mode_at = now;
    // Do not send a delayed temperature change after selecting Off/another
    // mode.
    target_pending = false;
    command_failed = false;
    return true;
  }
  bool target_due(uint32_t now) const {
    return ready() && target_pending && !target_sent &&
           uint32_t(now - target_at) >= 250;
  }
  void mark_target_sent(uint32_t now) {
    target_sent = true;
    target_at = now;
  }
  bool expire(uint32_t now) {
    bool expired = false;
    if (target_pending && target_sent && uint32_t(now - target_at) >= 5000) {
      target_pending = false;
      expired = true;
    }
    if (mode_pending && uint32_t(now - mode_at) >= 5000) {
      mode_pending = false;
      expired = true;
    }
    if (expired)
      command_failed = true;
    return expired;
  }
};

} // namespace esphome::round_thermostat
