#include "BLEScanner.h"

BLEScanner bleScanner;

#ifdef ESP32
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        ble_device_t dev;
        
        if (advertisedDevice.haveName()) {
            dev.name = String(advertisedDevice.getName().c_str());
        } else {
            dev.name = "Unknown";
        }
        
        dev.address = String(advertisedDevice.getAddress().toString().c_str());
        dev.rssi = advertisedDevice.getRSSI();
        
        if (advertisedDevice.getAddressType() == BLE_ADDR_TYPE_PUBLIC) {
            dev.type = "Public";
        } else if (advertisedDevice.getAddressType() == BLE_ADDR_TYPE_RANDOM) {
            dev.type = "Random";
        } else {
            dev.type = "Unknown";
        }
        
        dev.lastSeen = millis();
        
        if (!bleScanner.updateDevice(dev.address, dev)) {
            bleScanner.addDevice(dev);
        }
    }
};

static MyAdvertisedDeviceCallbacks deviceCallbacks;
#endif

BLEScanner::BLEScanner() {
    scanning = false;
}

void BLEScanner::start(uint32_t duration) {
    #ifdef ESP32
    stop();
    
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    if (pBLEScan) {
        pBLEScan->setAdvertisedDeviceCallbacks(&deviceCallbacks);
        pBLEScan->setActiveScan(true);
        pBLEScan->setInterval(100);
        pBLEScan->setWindow(99);
        
        scanning = true;
        scanDuration = duration;
        scanStartTime = millis();
        devices.clear();
        
        pBLEScan->start(duration / 1000, false);
    }
    #endif
}

void BLEScanner::stop() {
    #ifdef ESP32
    if (scanning) {
        BLEDevice::getScan()->stop();
        scanning = false;
    }
    #endif
}

void BLEScanner::update() {
    #ifdef ESP32
    if (scanning && (millis() - scanStartTime >= scanDuration)) {
        stop();
    }
    #endif
}

bool BLEScanner::isScanning() {
    return scanning;
}

uint16_t BLEScanner::getDeviceCount() {
    return devices.size();
}

ble_device_t* BLEScanner::getDevice(uint16_t index) {
    if (index < devices.size()) {
        return &devices.get(index);
    }
    return nullptr;
}

String BLEScanner::getDevicesJSON() {
    String json = "[";
    for (int i = 0; i < devices.size(); i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"name\":\"" + devices.get(i).name + "\",";
        json += "\"addr\":\"" + devices.get(i).address + "\",";
        json += "\"rssi\":" + String(devices.get(i).rssi) + ",";
        json += "\"type\":\"" + devices.get(i).type + "\"";
        json += "}";
    }
    json += "]";
    return json;
}

void BLEScanner::clear() {
    devices.clear();
}

void BLEScanner::addDevice(ble_device_t dev) {
    if (devices.size() < BLE_MAX_DEVICES) {
        devices.add(dev);
    }
}

bool BLEScanner::updateDevice(String address, ble_device_t dev) {
    for (int i = 0; i < devices.size(); i++) {
        if (devices.get(i).address == address) {
            devices.get(i) = dev;
            return true;
        }
    }
    return false;
}
