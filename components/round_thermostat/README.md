# Round Thermostat external component

This ESPHome component contains the UI from
[Pippowicz/ESPHome-Thermostat-Knob](https://github.com/Pippowicz/ESPHome-Thermostat-Knob),
extracted from commit `1bc92d16ef3086ac7bc8553ac9d7f079ff7cf4cb`.
It is designed for the existing 240 × 240 Elecrow ESP32-S3 knob configuration.

## Installation

Copy the updated `esphome-round-thermostat.yaml` and the complete
`components/round_thermostat/` directory into the ESPHome configuration directory:

```text
esphome/
├── esphome-round-thermostat.yaml
├── secrets.yaml
└── components/
    └── round_thermostat/
        ├── __init__.py
        ├── round_thermostat.h
        └── round_thermostat.cpp
```

The supplied YAML uses a local external component, so no component download is
needed at build time. Fonts and MDI icons retain their existing remote sources.
The firmware runs locally, as before.

Keep your existing secrets. Compile with ESPHome 2026.9.0:

```sh
esphome config esphome-round-thermostat.yaml
esphome compile esphome-round-thermostat.yaml
```

For a Git source, replace only the `external_components` block with a source
pointing to a branch/tag/commit that contains this component. Pin a tested commit
for reproducible builds:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/Pippowicz/ESPHome-Thermostat-Knob
      ref: YOUR_TESTED_COMMIT
    components: [round_thermostat]
```

## Responsibilities

The component owns:

- All screen drawing: temperature arc and marker, status icons, radial menu,
  standby screen and notification popup.
- Per-instance PSRAM lookup buffers, framebuffer invalidation and UI state.
- Touch hit testing and encoder behavior.
- Inactivity timing, standby brightness and wake-up brightness restoration.
- The 500 ms notification acknowledgement pulse.

The YAML retains hardware drivers and pins, fonts, images, Wi-Fi/API/OTA settings,
the RGB light, and all existing Home Assistant entities and persistence settings.
Every required entity/font/image is bound through a typed ID in
`round_thermostat:`; the C++ component does not depend on generated global IDs.

## Event wiring

The example YAML includes all required calls:

| Event | Method |
| --- | --- |
| Boot after entity restoration | `start()` |
| Display writer | `render(it)` |
| Touch | `touch(touch.x, touch.y)` |
| Encoder clockwise / anticlockwise | `rotate(1)` / `rotate(-1)` |
| Climate target action | `set_target_temperature(x)` |
| Climate mode or layout change | `redraw(true)` |
| Room temperature/humidity update | `redraw()` |
| Notification text update | `notification_changed()` |
| Standby brightness update | `standby_brightness_changed(x)` |

Draw requests are combined and executed on the next component loop, after the
entity actions finish. The inactivity check runs once a second, as in the
original YAML. Startup guards prevent early restored-entity callbacks from
rendering before the boot initialization completes.

## Preserved behavior

- Temperature range 5–30 °C and encoder steps of 0.5/1.0 °C.
- The menu is drawn with a 32 px inner radius; its intentionally smaller active
  area starts at radius 42 px.
- The first touch or encoder step in standby only wakes the display.
- Encoder changes remain possible while the mode menu is visible, and are
  ignored while a notification popup is open.
- Notification wrapping retains the original byte-based algorithm and six-line
  limit. Improved wrapping remains a separate future change.
- Notification acknowledgement does not clear the notification text or flag.
- An open notification popup prevents automatic standby.
- No changes to heating-source coordination or bidirectional HA synchronization.

## Hardware acceptance checks

The extracted component has been tested successfully on the project owner's knob.
Repeat these checks when changing firmware behavior:

1. Reboot and confirm restored climate mode, target and display settings.
2. Compare normal, off, standby, radial-menu and notification screens.
3. Test both encoder directions, both step sizes, and 5/30 °C limits.
4. Test all menu segments, including the deliberate 32–42 px inactive band.
5. Test standby timeout 0, normal timeout, brightness adjustment and wake-up.
6. Update temperature, humidity, mode and icons from Home Assistant.
7. Open and acknowledge a notification; confirm the approximately 500 ms pulse.
8. Confirm the RGB ring and its pulse effect still work after reboot.

## Attribution and license

ESPHome Round Thermostat / ESPHome-Thermostat-Knob by Pippowicz.
The original repository declares CC BY-NC 4.0 (Attribution-NonCommercial 4.0
International); the extracted code retains that license.

The original YAML was developed with OpenAI ChatGPT and tested on real hardware,
as documented upstream. This component extraction was AI-assisted and subsequently tested successfully
by the project owner on the physical knob.
