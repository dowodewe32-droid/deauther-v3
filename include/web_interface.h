#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <Arduino.h>

void start_web_interface();
void web_interface_handle_client();

#ifdef EVIL_TWIN_ENABLED
extern bool is_evil_twin_active();
extern void start_evil_twin(int target_network, const String& fake_ssid, const String& fake_pass);
extern void stop_evil_twin();
extern int captured_credentials_count;
#endif

#endif