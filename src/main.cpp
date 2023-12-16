/*******************************************************************************
 * ESP32 Sonos Remote (XBox)
 *
 * Provides a WiFi service to change Sonos volume via an XBox media (IR) remote.
 *
 * Use config.h to set up WiFi and peripheral hardware.
 *
 * Requires an IR receiver (TSOP38238 or similar) wired to a digital pin
 *
 * Thanks to piis3's sonos-buttons project for the Sonos library:
 * https://github.com/piis3/sonos-buttons
 *
 * Only volume up, down, and mute are implemented, and I did not include any
 * power saving features, as I am using a wired power supply.
 *
 * Copyright 2023 -- Yuri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#include <Arduino.h>
#include <Preferences.h>
#define LED_BUILTIN 2  // before IRremote.hpp if blink on IR receive is desired
#include <IRremote.hpp>

#include "config.h"
#include "sonos.h"

constexpr auto VOLUME_UP = 0x10;
constexpr auto VOLUME_DOWN = 0x11;
constexpr auto VOLUME_MUTE = 0xE;

static IPAddress targetSonos;

boolean connectWifi() {
    WiFi.mode(WIFI_STA);
    Serial.println("Connecting WiFi");
    WiFi.begin(SSID, PASSWORD);

    Serial.println("Waiting for WiFi to connect...");
    uint8_t wifiTries = 0;
    wl_status_t status = WiFi.status();
    while (status != WL_CONNECTED && status != WL_CONNECT_FAILED &&
           wifiTries < 50) {
        delay(50);
        status = WiFi.status();
        wifiTries += 1;
    }

    if (!WiFi.isConnected()) {
        Serial.println("WiFi connect failed, restarting");
        delay(1000);
        ESP.restart();
    }
    Serial.println("WiFi connect succeeded");
    return true;
}

void doSonos(int (*operation)(HTTPClient *http, IPAddress targetSonos)) {
    if (!targetSonos) {
        targetSonos = discoverSonos(std::string(SONOS_UID));
    }
    if (!targetSonos) {
        Serial.println("Couldn't discover Sonos, aborting!");
        return;
    }

    int error = sonosOperation(operation, targetSonos);
    if (error) {
        targetSonos = discoverSonos(std::string(SONOS_UID));
    }
}

void doSonosCommand(uint16_t command) {
    switch (command) {
        case VOLUME_UP:
            doSonos(volumeUp);
            break;
        case VOLUME_DOWN:
            doSonos(volumeDown);
            break;
        case VOLUME_MUTE:
            doSonos(volumeMute);
            break;
    }
}

void setup() {
    Serial.begin(115200);
    connectWifi();
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
    Preferences prefs;
    prefs.begin("sonos", true);
    String addr = prefs.getString("playerAddress", "");
    String prefUid = prefs.getString("playerUid", "");
    prefs.end();
    if (prefUid != String(SONOS_UID)) {
        prefs.begin("sonos", false);
        prefs.remove("playerAddress");
        prefs.end();
        addr = String("");
    }
    if (addr.length() > 0) {
        IPAddress ip;
        ip.fromString(addr.c_str());
        if (ip) {
            targetSonos = ip;
            Serial.printf("Using cached sonos IP %s",
                          targetSonos.toString().c_str());
        }
    }
    if (!targetSonos) {
        targetSonos = discoverSonos(std::string(SONOS_UID));
    }
}

void loop() {
    if (IrReceiver.decode()) {
        IrReceiver.printIRResultShort(&Serial);
        if (IrReceiver.decodedIRData.address == 0xD880) {
            doSonosCommand(IrReceiver.decodedIRData.command);
        }
        IrReceiver.resume();
    }
}
