# ESPHome Round Thermostat

A local Home Assistant thermostat interface for the **Elecrow ESP32-S3 1.28" HMI IPS Knob with Touch**.

Set the desired room temperature with a rotary encoder, select an operating mode
on the touchscreen, and display room conditions and notifications. The UI is
implemented as an ESPHome external component; hardware and Home Assistant
entities are configured in YAML.

The device YAML loads the display component, icons and fonts from Git. You only
need that YAML and your secrets locally; optional substitutions customize the
assets without editing the packages. The device YAML includes German comments
explaining configuration, hardware and UI connections.

<p align="center">
  <img src="images/thermostat-main.jpg" alt="Main thermostat screen with temperature arc" width="420">
</p>

## Contents

- [Purpose and current scope](#purpose-and-current-scope)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Migrating an existing configuration](#migrating-an-existing-configuration)
- [Understanding the device YAML](#understanding-the-device-yaml)
- [Thermostat data source](#thermostat-data-source)
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
selection, radiator setback and fallback behavior are not implemented here.
In standalone mode, selecting Auto, Heat or Cool only changes the knob's climate
entity. The optional linked mode directly controls one existing HA climate
entity; it does not implement the planned coordination between heating sources.
The heating-source selection currently changes a display icon.

Runtime communication uses the local ESPHome API. Building the firmware may
require internet access to download dependencies, fonts, icons and, when using
the Git installation option, the external component and YAML packages.

## Features

- Round 240 × 240 GC9A01A display and CST816D capacitive touchscreen.
- Standalone rotary adjustment from **5 to 30 °C**, in **0.5 or 1.0 °C** steps;
  linked mode respects the source thermostat's limits and step.
- Optional bidirectional control of an existing HA climate entity.
- Git-loaded icon/font packages with defaults and individual substitutions.
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

### 2. Choose a source

The supplied YAML downloads the external component and both asset packages from
GitHub. Only the device YAML and your `secrets.yaml` are needed locally.

```yaml
substitutions:
  thermostat_entity: "false"
  project_ref: development
```

`project_ref` selects the same branch or full commit SHA for
`external_components` and `packages`. Use `development` for testing these changes.
Use `main` only after the packages have been merged there.
`refresh: 0s` checks Git on each validation/build. Running devices only change
after compiling and flashing.

For a local copy, replace both source blocks with:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [round_thermostat]

packages:
  thermostat_images: !include packages/images.yaml
  thermostat_fonts: !include packages/fonts.yaml
```

Copy `components/round_thermostat/` and `packages/` alongside your device YAML.
Keep the `round_thermostat:` ID mappings unchanged. Loading only
`external_components` does not load YAML packages.

### Customize icons and fonts

The packages provide all defaults; no icon or font substitutions are required
in your device YAML. Override only what you want to change in the existing
`substitutions:` block:

```yaml
substitutions:
  thermostat_entity: "false"
  project_ref: development
  humidity_icon: "mdi:water-percent"
  humidity_icon_size: "32x32"
  font_file: "gfonts://Roboto"
  thermostat_font_size: "40"
```

Omitted settings keep their package defaults. `font_file` selects the common
font; individual `<font_id>_file` settings override it for one role.
See [packages/README.md](packages/README.md) for every setting and default.
The original icons, sizes, fonts and glyph sets are preserved by default.
Larger assets do not move surrounding elements; check for clipping or overlap
on the fixed 240 × 240 layout after flashing.

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

## Migrating an existing configuration

Keep your existing secrets, room/device names and chosen `thermostat_entity`.
The simplest migration is to copy the new base YAML and reapply those settings.
If your YAML has other customizations, migrate it in place:

1. Add `project_ref: development` to the existing `substitutions:` block.
2. Copy the new `external_components:` and `packages:` blocks from the base YAML.
3. Remove the old inline `image:` and `font:` blocks, which are now supplied by
   the packages. Retain any unrelated custom assets with unique IDs.
4. Convert your customized icons/fonts into substitutions using the
   [complete defaults table](packages/README.md). Leave the component ID mappings intact.
5. Validate, compile and flash. Check the main screen, menu, standby and a
   notification, especially if you changed asset sizes or fonts.

Do not merge another top-level `substitutions:` block into an existing one:
add the entries to the block already present. Existing installations continue
running their old firmware until you flash an update.

## Understanding the device YAML

| Block | Purpose | Usually customize? |
| --- | --- | --- |
| `substitutions` | Climate source, Git revision and optional asset overrides | Yes |
| `external_components` | Downloads the C++ UI component | Only for local installation |
| `packages` | Downloads icon/font definitions and defaults | Only for local installation |
| `round_thermostat` | Connects the UI to the configured entity, display and asset IDs | Usually keep mappings |
| `esphome`, `wifi`, `api`, `ota` | Device identity, connectivity and updates | Names and secrets |
| `esp32`, `psram`, `i2c`, `spi`, `display`, `touchscreen`, `output` | Board hardware and drivers | Only for different hardware |
| `sensor` | Rotary encoder events | Usually keep |
| `climate` | Knob's local thermostat entity and UI command callbacks | Room name |
| `number`, `text`, `select`, `switch`, `binary_sensor`, `light` | Display inputs, settings, status, notifications and lights | Use through HA |

In `room_humidity_icon: room_humidity_icon`, the left side is a component
parameter and the right side is an ESPHome ID. The actual icon is configured
in the package. Change `humidity_icon` or `humidity_icon_size` in substitutions
to customize it; changing the binding is unnecessary.

Even when linked to HA, `thermostat_climate` is the local knob entity, while
`source_entity` is the HA entity being controlled. They serve different roles
and must not reference the knob's own HA entity as its remote source.

## Thermostat data source

Choose the mode with the substitution at the top of the YAML, then compile and
flash. This is a build-time selection, not a runtime dropdown.

### Standalone (default)

```yaml
substitutions:
  thermostat_entity: "false"
```

This retains the original independent climate entity, restored setpoint, 5–30 °C
range and writable temperature/humidity display inputs. Existing HA automations
can keep using those inputs. No remote climate state subscriptions or control
actions are created. A YAML boolean `false` is also accepted; the quoted form
makes the substitution explicit.

### Link an existing Home Assistant thermostat

```yaml
substitutions:
  thermostat_entity: "climate.hmip_heating_int0000008"
```

Use the actual entity ID of the thermostat to control. **Do not select the knob's
own climate entity**, and remove any separate two-way synchronization automation
between the same two entities to avoid competing controllers.

For testing this feature, load both the YAML and component from `development`.
When loading from Git, set `project_ref: development` in `substitutions`.

The component subscribes to the selected entity through the local ESPHome API:

| HA state / attribute | Use |
| --- | --- |
| Entity state | Operating mode: `off`, `heat`, `cool`, `auto` |
| `temperature` | Single target temperature |
| `current_temperature` | Room temperature |
| `current_humidity` | Room humidity, if the source provides it |
| `min_temp`, `max_temp` | Target limits and display arc range |
| `target_temp_step` | Target rounding; defaults to 0.5 °C if absent |
| `hvac_modes` | Allowed mode commands; unsupported menu labels are dimmed |

Turn the knob to send `climate.set_temperature`; select a mode to send
`climate.set_hvac_mode`. Changes made in HA or at the source thermostat return to
the knob automatically. Incoming reports update the local climate state directly
and never invoke another outbound command. An `off` report with a target of
4.5 °C, for example, is preserved as a source report rather than sent back as a
new room-temperature request.

In Home Assistant, open **Settings → Devices & services → ESPHome**, configure
the knob's integration entry, and enable **Allow the device to perform Home
Assistant actions**. Reading states alone does not require this option, but
controlling the source does. See the [ESPHome API documentation](https://esphome.io/components/api/#actions).

The existing `Isttemperatur` and `Luftfeuchtigkeit` number entities remain present
for compatibility, but are not used by the display in linked mode. The humidity
display switch still controls visibility. Window status, notifications, the
heating-source icon and RGB settings remain independent of the linked climate.

**Availability and command handling:**

- Missing or invalid temperature/humidity values display as `--` rather than
  using persisted standalone values. Humidity is not available on every climate
  entity; it is not inferred from unrelated room entities.
- A lost HA subscription shows `HA offline`; an unknown/unavailable source shows
  `HA wartet`. Commands are blocked until the source is available. Unsent commands
  are discarded on disconnect and are not replayed after reconnecting.
- Rapid temperature changes are combined for 250 ms before sending the latest
  value. The requested value is shown while waiting for HA confirmation.
- If confirmation does not arrive within five seconds, the knob restores the
  latest reported state and shows `HA Fehler`. Check HA action permissions and
  device availability. A late report still updates the values; a new command
  clears the error indicator.
- `HA Daten` means the single target or temperature limits are missing. `HA Modus`
  means the current source mode cannot be represented by this UI.

The current UI supports **Celsius, single-setpoint thermostats**. `heat_cool`
(two target temperatures), `dry` and `fan_only` are not mapped to Auto. A supported
mode can still be selected if offered by the source. Source limits and rounding
apply even when the encoder is configured for a different increment. The encoder
uses at least the source's step size so both directions remain usable.

Linking a physical radiator thermostat makes its actual setpoint authoritative,
including temporary setbacks. To keep a separate room wish for future hybrid
heating control, retain standalone mode or link a suitable virtual HA climate
entity instead.

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

In standalone mode, `Isttemperatur` and `Luftfeuchtigkeit` are writable template numbers, not onboard
sensor readings. Home Assistant must copy measurements into them. They are
display inputs and are not currently wired as the climate entity's measured
temperature/humidity.

In standalone mode, the knob preserves these inputs across restarts. It does not yet detect stale
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
| [esphome-round-thermostat.yaml](esphome-round-thermostat.yaml) | Commented device configuration, package imports, HA entities and UI wiring |
| [components/round_thermostat/](components/round_thermostat/) | ESPHome schema and C++ UI implementation |
| [packages/](packages/) | Icon and font definitions with overridable defaults |
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

For testing, use the YAML from `development` with `project_ref: development`
(or matching local component and package files). Review the
[hardware checklist](components/round_thermostat/README.md#hardware-acceptance-checks)
when firmware behavior changes.

To return to an earlier version, use its matching YAML, component and packages,
or set `project_ref` to that version's full commit SHA, then compile and flash
again. Revisions before the package extraction require their original YAML
with inline assets. A revert commit can also undo a change on the development branch.
Changing a Git reference alone does not alter the firmware already on the device.

## Known limitations and future work

- Notification text wraps by byte count rather than measured pixel width;
  long words and UTF-8 truncation need improvement.
- Encoder behavior while the mode menu is open may be refined.
- Standalone room-temperature and humidity inputs have no freshness detection.
  Linked mode detects HA disconnection and unavailable states, but cannot detect
  a source integration that continues reporting stale measurements as valid.
- Additional local sensors may be connected through the auxiliary I²C pins.
- Heating-source coordination and safe bidirectional synchronization are future
  Home Assistant work, including separating the desired room setpoint from a
  temporary radiator setback.

### Automated checks

The transport-independent synchronization state can be tested without hardware:

```sh
g++ -std=c++17 -Wall -Wextra -Werror tests/test_remote_climate_state.cpp -o /tmp/test-remote-climate
/tmp/test-remote-climate
```

Before accepting a linked-mode change, test startup, both directions of control,
missing humidity, HA disconnection/reconnection and denied HA action permission.
Confirm that switching the substitution back to `"false"` preserves standalone
operation.

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
