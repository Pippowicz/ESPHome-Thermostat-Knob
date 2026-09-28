# Display asset packages

The base YAML loads `images.yaml` and `fonts.yaml` from Git using `project_ref`.
Device substitutions override package defaults. You do not need to copy any of
the defaults below into your device YAML.

## Icons

| Substitution | Default | Size substitution | Default size |
| --- | --- | --- | --- |
| `mode_auto_icon` | `mdi:refresh` | `mode_auto_icon_size` | `28x28` |
| `mode_heat_icon` | `mdi:fire` | `mode_heat_icon_size` | `28x28` |
| `mode_cool_icon` | `mdi:snowflake` | `mode_cool_icon_size` | `28x28` |
| `mode_off_icon` | `mdi:circle-off-outline` | `mode_off_icon_size` | `28x28` |
| `standby_mode_auto_icon` | `mdi:refresh` | `standby_mode_auto_icon_size` | `42x42` |
| `standby_mode_heat_icon` | `mdi:fire` | `standby_mode_heat_icon_size` | `42x42` |
| `standby_mode_cool_icon` | `mdi:snowflake` | `standby_mode_cool_icon_size` | `42x42` |
| `standby_mode_off_icon` | `mdi:circle-off-outline` | `standby_mode_off_icon_size` | `42x42` |
| `room_temperature_icon` | `mdi:thermometer` | `room_temperature_icon_size` | `28x28` |
| `humidity_icon` | `mdi:water` | `humidity_icon_size` | `28x28` |
| `heat_source_heatpump_icon` | `mdi:fan` | `heat_source_heatpump_icon_size` | `28x28` |
| `heat_source_radiator_icon` | `mdi:radiator` | `heat_source_radiator_icon_size` | `28x28` |
| `heat_source_ac_icon` | `mdi:air-conditioner` | `heat_source_ac_icon_size` | `28x28` |
| `window_open_icon` | `mdi:window-open-variant` | `window_open_icon_size` | `28x28` |
| `notification_icon` | `mdi:exclamation-thick` | `notification_icon_size` | `28x28` |

Each icon can be changed independently, including standby icons. Values accept
ESPHome file-image sources, such as `mdi:water-percent`. Keep the generated
image IDs and the component mappings unchanged. Images retain BINARY format
and chroma-key transparency.

## Fonts

| Substitution | Default |
| --- | --- |
| `font_file` | `gfonts://Roboto` |
| `thermostat_font_file` | `${font_file}` |
| `thermostat_font_size` | `38` |
| `standby_temperature_font_file` | `${font_file}` |
| `standby_temperature_font_size` | `57` |
| `room_temperature_font_file` | `${font_file}` |
| `room_temperature_font_size` | `18` |
| `menu_font_file` | `${font_file}` |
| `menu_font_size` | `16` |
| `notification_font_file` | `${font_file}` |
| `notification_font_size` | `18` |

`font_file` sets the default font for every role. Each `*_font_file` can select
a different font source independently, for example `gfonts://Open Sans`.
The menu and notification glyph sets remain unchanged, including German
characters. Choose a font that provides the required characters.

## Example

Add only overrides to the device's existing substitutions block:

```yaml
substitutions:
  thermostat_entity: "false"
  project_ref: development
  humidity_icon: "mdi:water-percent"
  humidity_icon_size: "32x32"
  thermostat_font_size: "40"
```

Without `humidity_icon_size`, its default remains 28x28; without any overrides,
the original display assets are used. Font sizes are pixels; icon sizes use
`WIDTHxHEIGHT`. The screen layout is fixed: larger assets can overlap or clip.
Compile and flash to apply changes.

For local packages and migration from an older YAML, see the
[root installation guide](../README.md#2-choose-a-source).
When migrating manually, remove the old `image:` and `font:` blocks before
adding the packages, to avoid duplicate IDs.
