#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <Arduino.h>

void start_web_interface();
void web_interface_handle_client();
extern int captured_credentials_count;

#endif