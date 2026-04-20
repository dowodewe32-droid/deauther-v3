#ifndef HTOOL_WEB_H
#define HTOOL_WEB_H

#include <stdint.h>

void htool_web_init();
void htool_web_handle_client();

bool htool_web_is_deauther_enabled();
bool htool_web_is_beacon_enabled();
bool htool_web_is_evil_twin_enabled();
bool htool_web_is_captive_portal_enabled();

void htool_web_start_deauther();
void htool_web_stop_deauther();
void htool_web_start_beacon(uint8_t num);
void htool_web_stop_beacon();
void htool_web_start_evil_twin(uint8_t ssid_index);
void htool_web_stop_evil_twin();
void htool_web_start_captive_portal(uint8_t cp_index);
void htool_web_stop_captive_portal();

int htool_web_get_scan_count();
void htool_web_get_scan_result(int index, char* ssid, int8_t* rssi, uint8_t* channel);

#endif