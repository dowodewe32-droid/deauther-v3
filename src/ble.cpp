#include "ble.h"
#include "definitions.h"

#ifdef ESP32
#include <BLEDevice.h>
#include <BLEScan.h>

BLEScan* bleScanner = nullptr;
String bleResults = "[]";

class MyCallback : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice* d) {
    String n = d->getName().c_str();
    if (n.length() == 0) n = "Unknown";
    bleResults = "[\"name\":\"" + n + "\",\"addr\":\"" + d->getAddress().toString() + "\"}]";
  }
};

void start_ble_scan() {
  BLEDevice::init("");
  bleScanner = BLEDevice::getScan();
  bleScanner->setAdvertisedDeviceCallbacks(new MyCallback());
  bleScanner->setActiveScan(true);
  bleScanner->start(BLE_SCAN_TIME);
  DEBUG_PRINTLN("BLE Scan Started");
}

void stop_ble_scan() {
  if (bleScanner) bleScanner->stop();
}

String get_ble_results() {
  return bleResults;
}
#else
void start_ble_scan() {}
void stop_ble_scan() {}
String get_ble_results() { return "[]"; }
#endif