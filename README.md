_"Whatever you try to do on arduino, you end up with a weather station or an alarm clock" - Arduino forums._

## ESP32 NTP synced clock with SHT30 temperature and humidity sensor

### Features

- NTP synced clock with LCD1602 display;
- Temperature + humidity sensor SHT30;
- Command line interface over Serial;
- Command line interface over Telnet;
- WiFi scanner (output to Serial or Trlnet);
- other (list on "help" command)...

#### Hardware
- ESP32-S3 N16R8
- LCD1602 display
- SHT30 sensor
- 4 tactile buttons

#### Libraries
- [LiquidCrystal_I2C_ESP32](https://github.com/iakop/LiquidCrystal_I2C_ESP32)
- [Rob Tillaart SHT85](https://github.com/RobTillaart/SHT85)
- [Lennart Hennigs](https://github.com/LennartHennigs/ESPTelnet)
- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
