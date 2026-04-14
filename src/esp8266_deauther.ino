/* =====================
   This software is licensed under the MIT License:
   https://github.com/spacehuhntech/esp8266_deauther
   ===================== */

#ifdef ESP32
    #include <WiFi.h>
    #include <esp_system.h>
#else
extern "C" {
  #include "user_interface.h"
}
#endif

#include "EEPROMHelper.h"

#include <ArduinoJson.h>
#if ARDUINOJSON_VERSION_MAJOR != 5
#error Please upgrade/downgrade ArduinoJSON library to version 5!
#endif

#include "oui.h"
#include "language.h"
#include "functions.h"
#include "settings.h"
#include "Names.h"
#include "SSIDs.h"
#include "Scan.h"
#include "Attack.h"
#include "CLI.h"
#include "BLEScanner.h"
#include "DisplayUI.h"
#include "A_config.h"

#include "led.h"

Names names;
SSIDs ssids;
Accesspoints accesspoints;
Stations     stations;
Scan   scan;
Attack attack;
CLI    cli;
DisplayUI displayUI;
BLEScanner bleScanner;

simplebutton::Button* resetButton;

#include "wifi.h"

uint32_t autosaveTime = 0;
uint32_t currentTime  = 0;

bool booted = false;

#ifdef ESP32
extern void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
#endif

void setup() {
#ifdef ESP32
    randomSeed(esp_random());
#else
    randomSeed(os_random());
#endif

    Serial.begin(115200);
    Serial.println();
    Serial.println("========== GMpro87dev BOOT ==========");
    
    Serial.println("[1] SPIFFS...");
    prnt(SETUP_MOUNT_SPIFFS);
    LittleFS.begin();
    prntln(SETUP_OK);
    Serial.println("[1] OK");

    Serial.println("[2] EEPROM...");
    EEPROMHelper::begin(EEPROM_SIZE);
    Serial.println("[2] OK");

    currentTime = millis();

    Serial.println("[3] Settings...");
    // Force reset to default SSID/Password
    settings::reset();
    // Set explicitly in case reset didn't work
    access_point_settings_t apSet = settings::getAccessPointSettings();
    strncpy(apSet.ssid, "GMpro", 32);
    strncpy(apSet.password, "Sangkur87", 64);
    apSet.hidden = false;
    settings::setAccessPointSettings(apSet);
    
    // Enable SPIFFS mode for web files
    web_settings_t ws = settings::getWebSettings();
    ws.use_spiffs = true;
    ws.enabled = true;
    settings::setWebSettings(ws);
    
    settings::save();
    Serial.print("[3] SSID: ");
    Serial.println(settings::getAccessPointSettings().ssid);
    Serial.print("[3] PASS: ");
    Serial.println(settings::getAccessPointSettings().password);
    Serial.println("[3] OK");

    Serial.println("[4] WiFi...");
    wifi::begin();
    Serial.println("[4] OK");

#ifdef ESP32
#else
    wifi_set_promiscuous_rx_cb([](uint8_t* buf, uint16_t len) {
        scan.sniffer(buf, len);
    });
#endif

#ifdef ESP32
    // Don't set promiscuous here - let scan.setup() handle it
#else
    wifi_set_promiscuous_rx_cb([](uint8_t* buf, uint16_t len) {
        scan.sniffer(buf, len);
    });
#endif

    if (settings::getDisplaySettings().enabled) {
        displayUI.setup();
        displayUI.mode = DISPLAY_MODE::INTRO;
    }

    names.load();
    ssids.load();
    cli.load();

    scan.setup();
    
    Serial.println("[5] CLI...");
    if (settings::getCLISettings().enabled) {
        cli.enable();
        Serial.println("[5] CLI enabled");
    } else {
        prntln(SETUP_SERIAL_WARNING);
        Serial.println("[5] CLI disabled - Serial will stay ON for debug");
        // DON'T end serial - keep it for debugging!
    }

    Serial.println("[6] Starting AP...");
    
    wifi::startAP();
    Serial.println("[6] AP started!");

    prntln(SETUP_STARTED);
    prntln(DEAUTHER_VERSION);
    Serial.println("========== BOOT COMPLETE ==========");

    led::setup();

    resetButton = new ButtonPullup(RESET_BUTTON);
}

void loop() {
    currentTime = millis();

    led::update();
    wifi::update();
    attack.update();
    displayUI.update();
    cli.update();
    scan.update();
    ssids.update();
    bleScanner.update();

    if (settings::getAutosaveSettings().enabled
        && (currentTime - autosaveTime > settings::getAutosaveSettings().time)) {
        autosaveTime = currentTime;
        names.save(false);
        ssids.save(false);
        settings::save(false);
    }

    if (!booted) {
        booted = true;
        EEPROMHelper::resetBootNum(BOOT_COUNTER_ADDR);
#ifdef HIGHLIGHT_LED
        displayUI.setupLED();
#endif
    }

    resetButton->update();
    if (resetButton->holding(5000)) {
        led::setMode(LED_MODE::SCAN);
        DISPLAY_MODE _mode = displayUI.mode;
        displayUI.mode = DISPLAY_MODE::RESETTING;
        displayUI.update(true);

        settings::reset();
        settings::save(true);

        delay(2000);

        led::setMode(LED_MODE::IDLE);
        displayUI.mode = _mode;
    }
}
