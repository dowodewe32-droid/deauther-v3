#ifndef BLE_H
#define BLE_H

#include <Arduino.h>

void start_ble_scan();
void stop_ble_scan();
String get_ble_results();
bool is_ble_running();

#endif