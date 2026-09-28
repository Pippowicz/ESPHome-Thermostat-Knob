#include "../components/round_thermostat/remote_climate_state.h"
#include <cassert>
#include <iostream>

using esphome::round_thermostat::RemoteClimateState;
using Mode = RemoteClimateState::Mode;

static RemoteClimateState connected_source() {
  RemoteClimateState state;
  state.connected = true;
  state.modes = "['off', 'heat', 'auto']";
  state.minimum = 5;
  state.maximum = 28;
  state.receive_mode("heat");
  state.receive_target(21);
  return state;
}

int main() {
  RemoteClimateState state;
  assert(!state.request_target(22, 0)); // No write before first HA report.
  assert(!state.request_mode(Mode::HEAT, 0));
  state = connected_source();
  assert(state.ready());
  assert(!state.target_pending &&
         !state.mode_pending); // Imports never queue writes.
  assert(state.supports(Mode::HEAT) && !state.supports(Mode::COOL));
  assert(!state.request_mode(Mode::COOL, 0));
  assert(!state.request_target(21, 0)); // Echo is a no-op.

  assert(state.request_target(21.5, 100));
  assert(state.request_target(22, 200)); // Rapid turning: last value wins.
  assert(!state.target_due(449) && state.target_due(450));
  state.mark_target_sent(450);
  assert(!state.target_due(451)); // No repeated send on every loop.
  state.receive_target(21.5);     // Late echo from a previous update.
  assert(state.target() == 22 && state.target_pending);
  state.receive_target(22);
  assert(!state.target_pending && state.target() == 22);
  assert(!state.request_target(22, 600));

  state.receive_target(20); // Wall thermostat / HA change.
  assert(state.target() == 20 && !state.target_pending);
  assert(state.request_target(40, 700) && state.target() == 28);
  state.mark_target_sent(950);
  assert(!state.expire(5949));
  assert(state.expire(5950)); // Denied HA action restores reported target.
  assert(state.target() == 20 && state.command_failed);

  state = connected_source();
  assert(state.request_target(22, 100));
  assert(state.request_mode(Mode::OFF, 150));
  assert(!state.target_due(400)); // No delayed setpoint after switching off.
  state.receive_mode("off");
  state.receive_target(
      4.5f); // Homematic off value is a report, not clamped to 5.
  assert(state.target() == 4.5f && !state.mode_pending);
  assert(!state.target_pending);

  state = connected_source();
  assert(state.request_mode(Mode::AUTO, 100));
  assert(state.expire(5100));
  assert(state.mode() == Mode::HEAT);
  assert(state.request_target(22, 6000));
  state.disconnect();
  assert(!state.ready() && !state.target_pending && !state.mode_pending);
  state.connected = true;
  assert(
      !state.request_target(23, 7000)); // Reconnection must import fresh state.
  assert(std::isnan(state.humidity) && std::isnan(state.temperature));

  state = connected_source();
  state.receive_mode("unavailable");
  assert(!state.ready() && std::isnan(state.target()));
  assert(!state.request_target(22, 0));
  state.receive_mode("heat_cool");
  assert(!state.ready()); // Never reinterpret dual-setpoint mode as auto.
  state.modes = "[\"heat_cool\", \"off\", \"heat\"]";
  assert(state.supports(Mode::HEAT));
  assert(!state.supports(Mode::AUTO));
  assert(
      state.request_mode(Mode::HEAT, 100)); // Can return to a supported mode.

  state = connected_source();
  state.minimum = NAN;
  assert(!state.request_target(22, 0));
  state.minimum = state.maximum;
  assert(!state.request_target(22, 0));
  state = connected_source();
  state.step = 1;
  assert(state.encoder_target(-1, 0.5f) == 20);
  assert(state.encoder_target(1, 0.5f) == 22);
  assert(state.request_target(21.5, 0) && state.target() == 22);
  assert(!state.request_target(NAN, 1));
  assert(!state.request_target(INFINITY, 1));

  state = connected_source();
  assert(state.request_target(22, UINT32_MAX - 100));
  assert(!state.target_due(100) && state.target_due(149)); // millis rollover.
  state.mark_target_sent(UINT32_MAX - 100);
  assert(state.expire(4899));
  std::cout << "Remote climate state regression checks passed\n";
}
