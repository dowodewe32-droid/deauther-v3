#pragma once

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

class Names;
class SSIDs;
class Accesspoints;
class Stations;
class Scan;
