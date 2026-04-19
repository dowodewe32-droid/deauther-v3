#include <WebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include "web_interface.h"
#include "definitions.h"
#include "deauth.h"
#include "web_ui_html.h"

extern void start_beacon();
extern void stop_beacon();
extern bool is_beacon_running();

WebServer server(80);
extern int eliminated_stations;
extern bool beacon_active;
extern int beacon_counter;

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
  if (t == "all") start_deauth(0, 1, 1);
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  stop_deauth();
  stop_beacon();
  server.send(200, "text/plain", "OK");
}

void handleBeacon() {
  String cmd = server.arg("cmd");
  if (cmd == "start") start_beacon();
  else if (cmd == "stop") stop_beacon();
  server.send(200, "text/plain", "OK");
}

void handleBeaconStatus() {
  String s = "{\"running\":";
  s += is_beacon_running() ? "true" : "false";
  s += ",\"count\":" + String(beacon_counter) + "}";
  server.send(200, "application/json", s);
}

void handleStatus() {
  String s = "{\"deauth\":" + String(eliminated_stations) + "}";
  server.send(200, "application/json", s);
}

void start_web_interface() {
  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/deauth", handleDeauth);
  server.on("/stop", handleStop);
  server.on("/beacon", handleBeacon);
  server.on("/beaconStatus", handleBeaconStatus);
  server.on("/status", handleStatus);
  server.begin();
  DEBUG_PRINTLN("Web Server Started");
}

void web_interface_handle_client() {
  server.handleClient();
}