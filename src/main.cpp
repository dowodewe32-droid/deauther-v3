#include <WiFi.h>
#include <esp_wifi.h>
#include "types.h"
#include "web_interface.h"
#include "deauth.h"
#include "evil_twin.h"
#include "definitions.h"

int curr_channel = 1;
unsigned long lastAPCheck = 0;

void setup() {
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
#endif
#ifdef LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
#endif

  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  start_web_interface();
}

void ensureAPMode() {
  if (WiFi.getMode() != WIFI_MODE_AP && WiFi.getMode() != (WIFI_MODE_AP | WIFI_MODE_STA)) {
    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
  }
}

void loop() {
#ifdef LED
  update_led_status();
#endif

  unsigned long now = millis();
  if (now - lastAPCheck > 5000) {
    ensureAPMode();
    lastAPCheck = now;
  }

  if (is_evil_twin_active()) {
    handle_evil_twin_client();
  } else if (deauth_type == DEAUTH_TYPE_ALL) {
    if (curr_channel > CHANNEL_MAX) curr_channel = 1;
    esp_wifi_set_channel(curr_channel, WIFI_SECOND_CHAN_NONE);
    curr_channel++;
    delay(10);
  } else {
    web_interface_handle_client();
  }
}