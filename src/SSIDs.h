/* This software is licensed under the MIT License: https://github.com/spacehuhntech/esp8266_deauther */

#pragma once

#include "Arduino.h"
#include <ESP8266WiFi.h>
#ifdef ESP32
    #include <WiFi.h>
#else
    extern "C" {
        #include "user_interface.h"
    }
#endif
#include "language.h"
#include "SimpleList.h"
#include "Accesspoints.h"
