#include <Arduino.h>
#include "ble.h"
#include "definitions.h"

bool ble_running = false;
String ble_results = "[]";

void start_ble_scan() {
  if (ble_running) return;
  ble_running = true;
  ble_results = "[]";
  DEBUG_PRINTLN("BLE scan started (placeholder - needs NimBLE or ESP32 BT stack)");
}

void stop_ble_scan() {
  ble_running = false;
  DEBUG_PRINTLN("BLE scan stopped");
}

String get_ble_results() {
  if (!ble_running) return "[]";
  return ble_results;
}

bool is_ble_running() {
  return ble_running;
}

void add_ble_device(const char* name, int rssi) {
  if (ble_results == "[]") {
    ble_results = "[{\"name\":\"" + String(name) + "\",\"rssi\":" + String(rssi) + "}]";
  } else {
    ble_results = ble_results + ",{\"name\":\"" + String(name) + "\",\"rssi\":" + String(rssi) + "}]";
  }
}