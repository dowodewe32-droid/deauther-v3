#ifndef EVIL_TWIN_H
#define EVIL_TWIN_H

#include <Arduino.h>
#include <WebServer.h>

void start_evil_twin(int target_network, const String& fake_ssid, const String& fake_pass);
void stop_evil_twin();
void handle_captive_portal();
void handle_login();
void handle_evil_twin_client();
bool is_evil_twin_active();
bool validate_password(const String& ssid, const String& password);
extern int captured_credentials_count;

#ifdef LED
extern void set_led_state(int state);
#endif

#endif