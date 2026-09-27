# ESPHome Round Thermostat

A local Home Assistant thermostat interface for the **Elecrow ESP32-S3 1.28" HMI IPS Knob with Touch**.

Set the desired room temperature with a rotary encoder, select an operating mode
on the touchscreen, and display room conditions and notifications. The UI is
implemented as an ESPHome external component; hardware and Home Assistant
entities are configured in YAML.

<p align="center">
  <img src="images/thermostat-main.jpg" alt="Main thermostat screen with temperature arc" width="420">
</p>

## Contents

- [Purpose and current scope](#purpose-and-current-scope)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Controls and screens](#controls-and-screens)
- [Home Assistant integration](#home-assistant-integration)
- [Hardware and pinout](#hardware-and-pinout)
- [Project structure](#project-structure)
- [Development and updates](#development-and-updates)
- [Known limitations and future work](#known-limitations-and-future-work)
- [AI-assisted development](#ai-assisted-development)
- [License](#license)

## Purpose and current scope

The knob provides a separate room-temperature setpoint and operating-mode
interface. Home Assistant can use these inputs to coordinate heating and cooling.

The intended application combines Homematic IP radiator heating, connected
locally through a CCU, with Daikin air conditioning connected through
[ESPHome Daikin S21](https://github.com/asund/esphome-daikin-s21). A future Home
Assistant automation can use surplus PV power for heating with the air
conditioner while reducing radiator demand.

**This repository currently implements the knob interface.** PV-based source
selection, radiator setback, fallback behavior and bidirectional setpoint
synchronization are not implemented here. Selecting Auto, Heat or Cool does not
by itself control either heating system. The heating-source selection currently
changes a display icon.

Runtime communication uses the local ESPHome API. Building the firmware may
require internet access to download dependencies, fonts, icons and, when using
the Git installation option, the external component.

## Features

- Round 240 × 240 GC9A01A display and CST816D capacitive touchscreen.
- Rotary temperature adjustment from **5 to 30 °C**, in **0.5 or 1.0 °C** steps.
- Home Assistant climate entity with Auto, Heat, Cool and Off modes.
- Radial touch menu and a custom antialiased 270° temperature arc.
- Room-temperature display and optional humidity display.
- Heating-source, open-window and notification indicators.
- Notification popup with a separate acknowledgement signal.
- Configurable standby timeout and backlight brightness.
- Five-LED RGB ring with static lighting and a pulsing effect.
- Restored climate mode, setpoint and configurable display settings after reboot.

## Requirements

The configuration has been built with **ESPHome 2026.9.0**, using the **ESP-IDF**
framework and **octal PSRAM at 80 MHz**. The extracted external component has
also been tested successfully on the project owner's physical knob.

Use the specified Elecrow board. Other ESP32-S3 display boards may need changes
to their GPIO assignments, display driver, touch configuration and PSRAM settings.

The display currently uses `ili9xxx` with the `GC9A01A` model. ESPHome 2026.9.0
reports this driver as deprecated; the project retains the tested configuration.

## Installation

### 1. Choose a branch

| Branch | Purpose |
| --- | --- |
| `main` | Accepted, hardware-tested project changes |
| `development` | Ongoing work to test before merging into `main` |

Copy [esphome-round-thermostat.yaml](esphome-round-thermostat.yaml) from the chosen
branch into your ESPHome configuration directory. Keep the YAML and component
from the same branch or commit.

### 2. Choose a component source

The supplied YAML uses a **local** component by default. Choose one of the
following options.

#### Option A: Load the component from GitHub

Replace the existing `external_components:` block with:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/Pippowicz/ESPHome-Thermostat-Knob
      ref: main
    components: [round_thermostat]
    refresh: 0s
```

To test ongoing work, change `ref: main` to `ref: development`.

Keep the following `round_thermostat:` block and all its ID mappings unchanged.
No local `components/` folder is needed with this option.

`refresh: 0s` checks the Git source on each validation/build. It does not update
a running device: compile and flash to install changes. For a fixed version,
replace the branch name with a tested full commit SHA.

#### Option B: Load the component locally

Keep the supplied `external_components:` block and copy the component directory
alongside the YAML:

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

This option uses the component files on disk rather than fetching them from Git.

### 3. Configure credentials and room name

Create `secrets.yaml` using [secrets.example.yaml](secrets.example.yaml):

```yaml
wifi_ssid: "YOUR_WIFI_SSID"
wifi_password: "YOUR_WIFI_PASSWORD"
fallback_hotspot_password: "YOUR_FALLBACK_AP_PASSWORD"
encryption_key: "YOUR_BASE64_ESPHOME_API_ENCRYPTION_KEY"
```

Use a valid ESPHome API encryption key. Keep `secrets.yaml` out of Git.

The example is named `temperaturregler-wohnzimmer` / `Temperaturregler Wohnzimmer`.
Adjust the device name, friendly name and climate entity name for your room.
The component's internal ID mappings can stay unchanged.

### 4. Validate, compile and flash

Use ESPHome Device Builder, or validate and compile from the command line:

```sh
esphome config esphome-round-thermostat.yaml
esphome compile esphome-round-thermostat.yaml
```

Flash using the method appropriate for your device, then add it through the
ESPHome integration in Home Assistant.

## Controls and screens

### Rotary encoder

Turn to change the target temperature. Select the increment with
`Temperatur Schrittweite`.

- In standby, the first encoder step wakes the screen without changing the target.
- While a notification popup is open, encoder input is ignored.
- While the radial menu is open, encoder input still changes the target.

### Touch menu

Touch the normal screen to open the radial menu. Select Auto, Heat, Cool or Off.
The notification segment is active only when a notification is present.

<p align="center">
  <img src="images/thermostat-mode-menu.jpg" alt="Radial operating-mode menu" width="380">
</p>

The visible inner circle has a radius of 32 pixels; the touch dead zone extends
to 42 pixels intentionally. Touching the central dead zone or outside the menu
closes it.

### Normal screen

The 270° arc runs from blue at the lower end of the range to red at the upper
end. A circular marker identifies the target temperature; the unused arc remains
dark. The center shows the target or `Off`.

Room temperature and optional humidity appear below. Status icons represent the
configured heating source, open-window state and pending notification.
The heating-source icon is shown in Heat and Auto modes.

### Standby

After the configured inactivity timeout, the display shows the operating-mode
icon, current room temperature (or `Off`) and any pending notification indicator.

<p align="center">
  <img src="images/thermostat-standby.jpg" alt="Standby screen" width="380">
</p>

A touch or encoder step wakes the screen without triggering another action.
Set `Timeout` to **0** to disable automatic standby. An open notification popup
also prevents automatic standby.

### Notifications

1. Home Assistant writes text to `Benachrichtigung`.
2. Home Assistant enables `Benachrichtigung vorhanden`.
3. Open the radial menu and select the notification segment.
4. Touch the popup to acknowledge it.

`Benachrichtigung bestätigt` turns on for approximately **500 ms**, then turns
off. Home Assistant can react to that signal. The knob leaves the text and
notification-present flag unchanged; clearing them is the automation's responsibility.

### RGB ring

`RGB Ring` controls the five WS2812 LEDs. Use its light controls for static color
and brightness, or select the **Pulsierend** effect. There is no separate
`RGB Ring Modus` entity in the current configuration.

## Home Assistant integration

Entity names are German in the example configuration. Actual entity IDs depend
on your Home Assistant setup.

| Entity | Type | Purpose |
| --- | --- | --- |
| Temperaturregler Wohnzimmer | Climate | Desired temperature and operating mode |
| Isttemperatur | Number | Room temperature supplied by an automation |
| Luftfeuchtigkeit | Number | Room humidity supplied by an automation |
| Temperatur Schrittweite | Select | Encoder increment: 0.5 or 1.0 °C |
| Luftfeuchtigkeit Anzeige | Select | Show or hide humidity |
| Heizquelle | Select | Choose heat-pump, radiator or air-conditioner icon |
| Fenster offen | Switch | Control the open-window indicator |
| Benachrichtigung | Text | Notification message |
| Benachrichtigung vorhanden | Switch | Enable the notification indicator/menu entry |
| Benachrichtigung bestätigt | Binary sensor | Brief acknowledgement pulse |
| Timeout | Number | Standby timeout in seconds; 0 disables it |
| Standby Helligkeit | Number | Standby brightness in percent |
| Display Hintergrundbeleuchtung | Light | Display backlight |
| RGB Ring | Light | RGB color, brightness and pulse effect |

`Isttemperatur` and `Luftfeuchtigkeit` are writable template numbers, not onboard
sensor readings. Home Assistant must copy measurements into them. They are
display inputs and are not currently wired as the climate entity's measured
temperature/humidity.

The knob preserves these inputs across restarts. It does not yet detect stale
measurements or indicate when Home Assistant has stopped updating them.

## Hardware and pinout

| Function | GPIO |
| --- | --- |
| Display SCLK | 10 |
| Display MOSI | 11 |
| Display DC | 3 |
| Display CS | 9 |
| Display reset | 14 |
| Display backlight | 46 |
| Touch SDA / SCL | 6 / 7 |
| Touch interrupt / reset | 5 / 13 |
| Rotary encoder A / B | 45 / 42 |
| Rotary encoder switch | 41 — not configured in this YAML |
| WS2812 RGB LEDs | 48 |
| Power indicator | 40 |
| Board OUT1 / OUT2 | 1 / 2 |
| Auxiliary I²C SDA / SCL | 38 / 39 — not configured in this YAML |

GPIO3, GPIO45 and GPIO46 produce strapping-pin warnings in the tested build;
these assignments follow the board's wiring. The CST816D uses its interrupt pin
and `skip_probe: true`.

The auxiliary I²C pins are available for future sensors such as a BME280.

## Project structure

| Path | Contents |
| --- | --- |
| [esphome-round-thermostat.yaml](esphome-round-thermostat.yaml) | Hardware, assets, HA entities and UI event wiring |
| [components/round_thermostat/](components/round_thermostat/) | ESPHome schema and C++ UI implementation |
| [Component documentation](components/round_thermostat/README.md) | API/event wiring and hardware test checklist |
| [secrets.example.yaml](secrets.example.yaml) | Credential template |
| [images/](images/) | Photos of the interface |

The external component owns rendering, touch handling, encoder behavior,
standby timing and the notification acknowledgement pulse. Supersampled arc
geometry is calculated once and stored in PSRAM lookup tables.

The display uses `update_interval: never` and `auto_clear_enabled: false`.
UI changes request redraws, which the component combines on its next loop.

## Development and updates

Changes are committed to **development** and proposed in a pull request to
**main**. The maintainer tests the development version on the knob and merges
the PR when the result is satisfactory.

For testing, use the YAML from `development` with `ref: development` (or matching
local component files). Review the
[hardware checklist](components/round_thermostat/README.md#hardware-acceptance-checks)
when firmware behavior changes.

To return to an earlier version, use its matching YAML and component files, or
pin the component's `ref` to that version's full commit SHA, then compile and
flash again. A revert commit can also undo a change on the development branch.
Changing a Git reference alone does not alter the firmware already on the device.

## Known limitations and future work

- Notification text wraps by byte count rather than measured pixel width;
  long words and UTF-8 truncation need improvement.
- Encoder behavior while the mode menu is open may be refined.
- Stale room-temperature and humidity inputs are not detected.
- Additional local sensors may be connected through the auxiliary I²C pins.
- Heating-source coordination and safe bidirectional synchronization are future
  Home Assistant work, including separating the desired room setpoint from a
  temporary radiator setback.

## AI-assisted development

The original ESPHome YAML was generated with OpenAI ChatGPT through an iterative
process of implementation, testing, debugging and refinement on real hardware.
The external-component extraction was also AI-assisted and subsequently tested
successfully by the project owner on the physical knob.

## License

This project is released under
[CC BY-NC 4.0 — Attribution-NonCommercial 4.0 International](https://creativecommons.org/licenses/by-nc/4.0/).

When redistributing the project or substantial portions of it, retain the project
name, copyright notices and license information.

**Attribution:** ESPHome Round Thermostat / ESPHome-Thermostat-Knob by Pippowicz.
