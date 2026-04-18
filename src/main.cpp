#include <WiFi.h>
#include <esp_wifi.h>
#include <DNSServer.h>
#include "types.h"
#include "web_interface.h"
#include "deauth.h"
#include "beacon.h"
#include "ble.h"
#include "definitions.h"

DNSServer dnsServer;
const byte DNS_PORT = 53;
int curr_channel = 1;

extern int eliminated_stations;
extern bool beacon_active;
extern int beacon_counter;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("========================");
  Serial.println("GMpro87dev - Full Features");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.println("========================");
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  start_web_interface();
  WiFi.scanNetworks(true);
  
  Serial.println("Ready!");
}

void loop() {
  dnsServer.processNextRequest();
  web_interface_handle_client();
  
  if (deauth_type == DEAUTH_TYPE_ALL) {
    if (curr_channel > CHANNEL_MAX) curr_channel = 1;
    esp_wifi_set_channel(curr_channel, WIFI_SECOND_CHAN_NONE);
    curr_channel++;
    delay(10);
  }
  
  update_beacon();
}