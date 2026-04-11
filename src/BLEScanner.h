#pragma once

#include "Arduino.h"
#ifdef ESP32
    #include <BLEDevice.h>
    #include <BLEScan.h>
    #include <BLEAdvertisedDevice.h>
#endif
#include "SimpleList.h"

#define BLE_SCAN_TIME 5000
#define BLE_MAX_DEVICES 50

typedef struct {
    String name;
    String address;
    int rssi;
    String type;
    uint32_t lastSeen;
} ble_device_t;

class BLEScanner {
public:
    BLEScanner();
    
    void start(uint32_t duration = BLE_SCAN_TIME);
    void stop();
    void update();
    
    bool isScanning();
    uint16_t getDeviceCount();
    ble_device_t getDevice(uint16_t index);
    String getDevicesJSON();
    
    void clear();
    void addDevice(ble_device_t dev);
    bool updateDevice(String address, ble_device_t dev);
    
private:
    #ifdef ESP32
    class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
        void onResult(BLEAdvertisedDevice advertisedDevice);
    };
    
    BLEScan* pBLEScan;
    #endif
    
    SimpleList<ble_device_t> devices;
    bool scanning;
    uint32_t scanStartTime;
    uint32_t scanDuration;
};

extern BLEScanner bleScanner;
