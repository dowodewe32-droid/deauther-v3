#ifndef WHITELIST_H
#define WHITELIST_H

#include <Arduino.h>

#define MAX_WHITELIST 50

typedef struct {
  uint8_t mac[6];
  char ssid[33];
  bool active;
} whitelist_entry_t;

extern whitelist_entry_t whitelist[MAX_WHITELIST];
extern int whitelist_count;

void init_whitelist();
bool is_whitelisted(const uint8_t* mac);
bool add_to_whitelist(const uint8_t* mac, const char* ssid);
bool remove_from_whitelist(const uint8_t* mac);
void clear_whitelist();
int get_whitelist_count();
String get_whitelist_json();

#endif