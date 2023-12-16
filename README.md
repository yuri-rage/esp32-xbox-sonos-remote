## XBox Sonos Remote (ESP32)

I wanted to control my Sonos surround system volume with an 8Bitdo XBox media remote. That remote does not provide any universal IR functionality, so it was up to me to decode the IR commands and find a way to use them with Sonos. I found that relaying the volume commands over WiFi was simple, reliable, and surprisingly responsive.

You'll need a TSOP38238 (or similar) IR receiver. I was able to solder it directly to the ESP32 header on the +5V, GND, and D13 pins.

Edit `config.h` with your WiFi details and IR receiver pin. My dev board included a built-in LED on pin 2, which is defined in `main.cpp` to blink on IR receive.

Per piis3's recommendation, I used the Python SoCo module to determine the Sonos UID:

```
>>> import soco
>>> [zone.uid for zone in soco.discover() if zone.player_name == 'Theater']
['RINCON_xxxxxxxxxxxxxxxxx']
```

I did not include any power saving features, as I am powering the ESP32 via
wired USB.

This project is structured for PlatformIO, though it should be trivial to convert to an Arduino IDE project by placing all source and header files into the same directory and renaming `main.cpp` to an appropriate `.ino` sketch filename.

Many thanks to piis3 for the [sonos-buttons](https://github.com/piis3/sonos-buttons) source that provided the basis for the included Sonos library.

`This project is provided under the MIT License.`