#include <WebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include "web_interface.h"
#include "definitions.h"
#include "deauth.h"
#include "beacon.h"
#include "web_ui_html.h"
#include "evil_twin.h"
#include "ble.h"
#include "whitelist.h"
#include "auth.h"

WebServer server(80);
int eliminated_stations = 0;
bool beacon_active = false;
int beacon_counter = 0;

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleScan() {
  int n = WiFi.scanComplete();
  if (n == -2) { WiFi.scanNetworks(true); server.send(200, "application/json", "[]"); }
  else if (n == -1) server.send(200, "application/json", "[]");
  else {
    String json = "[";
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"ch\":" + WiFi.channel(i) + ",\"rssi\":" + WiFi.RSSI(i) + "}";
    }
    json += "]";
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
    server.send(200, "application/json", json);
  }
}

void handleDeauth() {
  String t = server.arg("type");
  if (t == "all") {
    deauth_type = DEAUTH_TYPE_ALL;
    eliminated_stations = 0;
  }
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  deauth_type = DEAUTH_TYPE_SINGLE;
  stop_beacon();
  stop_evil_twin();
  server.send(200, "text/plain", "OK");
}

void handleBeacon() {
  String cmd = server.arg("cmd");
  if (cmd == "start") {
    start_beacon();
    beacon_active = true;
  } else if (cmd == "stop") {
    stop_beacon();
    beacon_active = false;
  }
  server.send(200, "text/plain", "OK");
}

void handleBeaconStatus() {
  String s = "{\"running\":";
  s += beacon_active ? "true" : "false";
  s += ",\"count\":" + String(beacon_counter) + "}";
  server.send(200, "application/json", s);
}

void handleEvilTwin() {
  String ssid = server.arg("ssid");
  String cmd = server.arg("cmd");
  if (cmd == "stop") {
    stop_evil_twin();
  } else if (ssid.length() > 0) {
    start_evil_twin(ssid.c_str());
  }
  String s = "{\"running\":";
  s += is_et_running() ? "true" : "false";
  s += ",\"clients\":" + String(get_et_client_count()) + "}";
  server.send(200, "application/json", s);
}

void handleBLE() {
  String cmd = server.arg("cmd");
  if (cmd == "start") {
    start_ble_scan();
  } else if (cmd == "stop") {
    stop_ble_scan();
  }
  String s = "{\"running\":\"" + String(is_ble_running() ? "true" : "false") + "\",\"devices\":" + get_ble_results() + "}";
  server.send(200, "application/json", s);
}

void handleWhitelistAdd() {
  String mac = server.arg("mac");
  String ssid = server.arg("ssid");
  if (mac.length() > 0 && ssid.length() > 0) {
    uint8_t m[6];
    int p[6];
    if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", &p[0], &p[1], &p[2], &p[3], &p[4], &p[5]) == 6) {
      for (int i = 0; i < 6; i++) m[i] = p[i];
      add_to_whitelist(m, ssid.c_str());
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleWhitelistClear() {
  clear_whitelist();
  server.send(200, "text/plain", "OK");
}

void handleWhitelistStatus() {
  server.send(200, "application/json", get_whitelist_json());
}

void handleLogin() {
  String user = server.arg("username");
  String pass = server.arg("password");
  String s = "{\"success\":" + String(validate_login(user.c_str(), pass.c_str()) ? "true" : "false") + "}";
  server.send(200, "application/json", s);
}

void handleStatus() {
  String s = "{\"deauth\":" + String(eliminated_stations) + ",\"beacon\":" + beacon_counter + ",\"etrunning\":" + String(is_et_running() ? "true" : "false") + ",\"blerunning\":" + String(is_ble_running() ? "true" : "false") + "}";
  server.send(200, "application/json", s);
}

void start_web_interface() {
  init_whitelist();
  
  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/deauth", handleDeauth);
  server.on("/stop", handleStop);
  server.on("/beacon", handleBeacon);
  server.on("/beaconStatus", handleBeaconStatus);
  server.on("/etwin", handleEvilTwin);
  server.on("/ble", handleBLE);
  server.on("/wl_add", handleWhitelistAdd);
  server.on("/wl_clear", handleWhitelistClear);
  server.on("/wl_status", handleWhitelistStatus);
  server.on("/login", handleLogin);
  server.on("/status", handleStatus);
  server.begin();
  DEBUG_PRINTLN("Web Server Started");
}

void web_interface_handle_client() {
  server.handleClient();
  handleDNSRequest();
}