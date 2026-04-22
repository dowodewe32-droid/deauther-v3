#ifndef DEAUTH_H
#define DEAUTH_H

#include <Arduino.h>
#include "definitions.h"

void start_deauth(int wifi_number, int attack_type, uint16_t reason);
void stop_deauth();

extern int eliminated_stations;
extern int deauth_type;

#ifdef LED
extern void set_led_state(int state);
#endif

#endif