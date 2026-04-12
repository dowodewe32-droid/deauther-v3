#include "wifi.h"

namespace wifi {
    // ===== PRIVATE ===== //
    wifi_mode_t mode;
    String path = "";
    String ap_ssid;
    String ap_password;
    uint8_t ap_channel = 1;
    bool ap_hidden = false;
    bool ap_captive_portal = true;

    IPAddress ip WEB_IP_ADDR;
    IPAddress netmask(255, 255, 255, 0);

    #ifdef ESP32
    WebServer server(80);
    DNSServer dns;
    #else
    ESP8266WebServer server(80);
    DNSServer dns;
    #endif

    // ===== SETTINGS ===== //
    void setPath(String p) {
        path = p;
        if (path.endsWith("/")) path.remove(path.length()-1);
    }

    void setSSID(String s) { ap_ssid = s; }
    void setPassword(String p) { ap_password = p; }
    void setChannel(uint8_t c) { ap_channel = c; }
    void setHidden(bool h) { ap_hidden = h; }
    void setCaptivePortal(bool c) { ap_captive_portal = c; }

    String getSSID() { return ap_ssid; }
    String getPassword() { return ap_password; }

    // ===== PUBLIC ===== //
    void begin() {
        #ifdef ESP32
        Serial.println("[WIFI] ESP32 initializing...");
        WiFi.mode(WIFI_OFF);
        delay(100);
        #endif
    }

    void startAP() {
        #ifdef ESP32
        Serial.println("========== STARTING AP ==========");
        Serial.print("SSID: ");
        Serial.println(ap_ssid);
        Serial.print("Password: ");
        Serial.println(ap_password);
        
        // Step 1: Set mode to AP
        WiFi.mode(WIFI_AP);
        delay(100);
        
        // Step 2: Configure IP (Marauder style)
        WiFi.softAPConfig(ip, ip, netmask);
        delay(100);
        
        // Step 3: Start softAP
        bool result = WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
        
        delay(500);
        
        Serial.print("softAP result: ");
        Serial.println(result ? "OK" : "FAILED");
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP());
        Serial.print("AP MAC: ");
        Serial.println(WiFi.softAPmacAddress());
        Serial.println("========== AP READY ==========");
        
        // Start DNS
        dns.start(53, "*", ip);
        
        // Simple web server
        server.on("/", [](){
            server.send(200, "text/html", 
                "<html><body>"
                "<h1>ESP32 Deauther V3</h1>"
                "<p>AP is working!</p>"
                "<p>SSID: " + ap_ssid + "</p>"
                "</body></html>");
        });
        server.begin();
        
        #else
        WiFi.softAPConfig(ip, ip, netmask);
        WiFi.softAP(ap_ssid.c_str(), ap_password.c_str(), ap_channel, ap_hidden);
        dns.start(53, "*", ip);
        server.begin();
        #endif
    }

    void stopAP() {
        #ifdef ESP32
        WiFi.softAPdisconnect(true);
        dns.stop();
        #else
        WiFi.softAPdisconnect(true);
        dns.stop();
        #endif
    }

    void update() {
        #ifdef ESP32
        dns.processNextRequest();
        server.handleClient();
        #else
        dns.processNextRequest();
        server.handleClient();
        #endif
    }
}
