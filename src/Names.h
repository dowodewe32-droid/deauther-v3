/* This software is licensed under the MIT License: https://github.com/spacehuhntech/esp8266_deauther */

#pragma once

#include "Arduino.h"
#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
    extern "C" {
        #include "user_interface.h"
    }
#endif
#include "language.h"
#include "SimpleList.h"

class Names {
    public:
        Names();

        void load();
        void save(bool eeprom=false);

        void add(const char* name);
        void remove(int i);

        void sort();
        void select(int i);
        void deselect(int i);
        void selectAll();
        void deselectAll();

        String get(int i);
        bool getSelected(int i);
        String getSelectedStr(int i);

        int find(const char* name);
        int count();
        int selected();

        bool check(int i);

        bool changed = false;

    private:
        SimpleList<char*>* list;
        SimpleList<String>* selectedlist;
        int8_t findPos(const char* name, int len, bool richtung, int startPos);
};
