#include <Arduino.h>
#include "whitelist.h"
#include "definitions.h"

whitelist_entry_t whitelist[MAX_WHITELIST];
int whitelist_count = 0;

void init_whitelist() {
  memset(whitelist, 0, sizeof(whitelist));
  whitelist_count = 0;
  DEBUG_PRINTLN("Whitelist initialized");
}

bool is_whitelisted(const uint8_t* mac) {
  for (int i = 0; i < whitelist_count; i++) {
    if (whitelist[i].active && memcmp(whitelist[i].mac, mac, 6) == 0) {
      return true;
    }
  }
  return false;
}

bool add_to_whitelist(const uint8_t* mac, const char* ssid) {
  if (whitelist_count >= MAX_WHITELIST) return false;
  
  for (int i = 0; i < whitelist_count; i++) {
    if (memcmp(whitelist[i].mac, mac, 6) == 0) {
      whitelist[i].active = true;
      return true;
    }
  }
  
  memcpy(whitelist[whitelist_count].mac, mac, 6);
  strncpy(whitelist[whitelist_count].ssid, ssid, 32);
  whitelist[whitelist_count].ssid[32] = 0;
  whitelist[whitelist_count].active = true;
  whitelist_count++;
  
  DEBUG_PRINT("Added to whitelist: ");
  DEBUG_PRINTLN(ssid);
  return true;
}

bool remove_from_whitelist(const uint8_t* mac) {
  for (int i = 0; i < whitelist_count; i++) {
    if (memcmp(whitelist[i].mac, mac, 6) == 0) {
      whitelist[i].active = false;
      DEBUG_PRINT("Removed from whitelist: ");
      DEBUG_PRINTLN(whitelist[i].ssid);
      return true;
    }
  }
  return false;
}

void clear_whitelist() {
  memset(whitelist, 0, sizeof(whitelist));
  whitelist_count = 0;
  DEBUG_PRINTLN("Whitelist cleared");
}

int get_whitelist_count() {
  int count = 0;
  for (int i = 0; i < whitelist_count; i++) {
    if (whitelist[i].active) count++;
  }
  return count;
}

String get_whitelist_json() {
  String json = "[";
  for (int i = 0; i < whitelist_count; i++) {
    if (whitelist[i].active) {
      if (i > 0) json += ",";
      json += "{\"mac\":\"";
      for (int j = 0; j < 6; j++) {
        if (j > 0) json += ":";
        json += String(whitelist[i].mac[j], HEX);
      }
      json += "\",\"ssid\":\"" + String(whitelist[i].ssid) + "\"}";
    }
  }
  json += "]";
  return json;
}