# ESPHome Round Thermostat

A custom ESPHome thermostat interface for the **Elecrow ESP32-S3 1.28" Round HMI Display with Touch**.

The project turns the round ESP32-S3 display into a Home Assistant-compatible climate controller with rotary encoder input, touch controls, operating-mode selection, standby display, status indicators and notifications.

<p align="center">
  <img src="images/thermostat-main.jpg" alt="ESPHome Round Thermostat main screen" width="420">
</p>

## Features

- 240 × 240 round GC9A01A display
- CST816D capacitive touchscreen
- Rotary encoder for changing the target temperature
- Home Assistant climate entity
- Heating, cooling, auto and off modes
- Radial touch menu
- Configurable temperature step: 0.5 °C / 1.0 °C
- Current room temperature display
- Optional humidity display
- Configurable heating-source indicator
- Window-open indicator
- Notification indicator and notification popup
- Notification acknowledgement entity for Home Assistant automations
- Configurable standby timeout and standby brightness
- RGB LED ring with static and pulsing modes
- Restores climate mode, target temperature and display settings after reboot
- Optimized custom-rendered 270° temperature arc using PSRAM lookup tables

---

## Hardware

Designed for the **Elecrow ESP32 1.28" HMI IPS Knob with Touch**, based on an ESP32-S3.

### Pinout

| Function | GPIO |
|---|---:|
| Display SCLK | GPIO10 |
| Display MOSI | GPIO11 |
| Display DC | GPIO3 |
| Display CS | GPIO9 |
| Display RESET | GPIO14 |
| Display backlight | GPIO46 |
| Touch SDA | GPIO6 |
| Touch SCL | GPIO7 |
| Touch INT | GPIO5 |
| Touch RESET | GPIO13 |
| Rotary encoder A | GPIO45 |
| Rotary encoder B | GPIO42 |
| Rotary encoder switch | GPIO41 |
| WS2812 RGB LEDs | GPIO48 |
| Power indicator | GPIO40 |
| Board OUT1 | GPIO1 |
| Board OUT2 | GPIO2 |
| Auxiliary I²C SDA | GPIO38 |
| Auxiliary I²C SCL | GPIO39 |

The auxiliary I²C connection can be used for future sensors such as a BME280.

---

## Software

The configuration was developed and tested with:

- **ESPHome 2026.9.0**
- ESP-IDF framework
- Home Assistant
- ESP32-S3 with octal PSRAM

The display currently uses ESPHome's `ili9xxx` component with the `GC9A01A` model.

> **Note:** ESPHome currently reports `ili9xxx` as deprecated. This project intentionally keeps the known-working implementation rather than changing the display driver unnecessarily.

---

## Installation

Copy:

```text
esphome-round-thermostat.yaml
