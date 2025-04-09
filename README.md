# Geiger

This is a simple radioactivity sensor. It connects to the pulse output of a Alibaba RadiationD-v1.1 (CAJOE) Geiger counter board. The controller is a Lolin32 (ESP32) with an OLED display on board.

It is advisable to wrap the J305 glass tube used in the counter with a black paper since it is sensitive to UV light. Gamma sensitivity will not be affected by this.

The controller counts the pulses and cumulates them in a sliding 60 second interval. A gliding threshold is also maintained. The node connects to a local WiFi network and publishes its measurements via MQTT; it also shows the current measurements on the display – that's it. All parameters are configured in the source.

Addinitionally, a true random number generator function is configurable. When 64 bytes (configurable) have been generated, they are published as a hex string. This needs 4 events per bit, so the time needed for one such published string depends on the current activity.

Hardware and software might become more elaborated in future versions (e.g. WiFiManager configuration, configuration via MQTT, …).

## Wiring

ESP32:
5V:         supply to Geiger counter
GND:        ground to Geiger counter
GPIO26:     pulse input from Geiger counter

GPIO04:     I2C_SCL (intern, for OLED)
GPIO05:     I2C_SDA (intern, for OLED)

– all others not connected.

