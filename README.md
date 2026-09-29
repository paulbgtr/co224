# co224

A small Wi-Fi CO₂ monitor. An ESP32-S3 reads CO₂, temperature and humidity from a Sensirion SCD4x sensor and publishes them over MQTT. A single-page web dashboard shows the live values in a browser.

```
SCD4x ──I²C──> ESP32-S3 ──MQTT/TLS──> broker (e.g. HiveMQ Cloud) ──WebSocket──> frontend/index.html
```

## MQTT topics

The device ID comes from the last three bytes of the Wi-Fi MAC address, e.g. `co2-a1b2c3`. The device logs it at boot.

| Topic | Payload | Notes |
|---|---|---|
| `co2/<device>/data` | `{"co2":812,"t":23.4,"rh":41.0}` | ppm, °C, %RH; QoS 1, retained |
| `co2/<device>/status` | `online` / `offline` | retained; `offline` is the last-will message |

## Firmware

Requires ESP-IDF (v5.x recommended). The `esp-idf-lib/scd4x` component is fetched automatically by the component manager.

Wiring: SDA → GPIO 5, SCL → GPIO 6.

```sh
cd firmware
idf.py set-target esp32s3
idf.py menuconfig      # "CO2 sensor" menu: Wi-Fi SSID/password, MQTT URI/user/password
idf.py build flash monitor
```

## Frontend

```sh
cd frontend
cp config.example.js config.js   # set broker WebSocket URL, credentials and device ID
```

Then open `index.html` in a browser or serve the folder with any static server. `config.js` is git-ignored, so credentials stay local.
