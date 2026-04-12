/* =====================
   ESP8266 DEAUTHER V3 - BRUCE STYLE INIT
   Uses Bruce ESP WiFi initialization pattern
   ===================== */

#ifdef ESP32
    #include <WiFi.h>
    #include <DNSServer.h>
    #include <WebServer.h>
    #include <esp_wifi.h>
#else
    #include <ESP8266WiFi.h>
    #include <DNSServer.h>
    #include <ESP8266WebServer.h>
#endif

// Settings
const char* ap_ssid = "GMpro";
const char* ap_password = "Sangkur87";
const IPAddress ap_ip(192, 168, 4, 1);
const IPAddress ap_gateway(192, 168, 4, 1);
const IPAddress ap_subnet(255, 255, 255, 0);

// Servers
#ifdef ESP32
    WebServer server(80);
    DNSServer dnsServer;
#else
    ESP8266WebServer server(80);
    DNSServer dnsServer;
#endif

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("==========================================");
    Serial.println("  ESP8266 DEAUTHER V3 - BRUCE STYLE");
    Serial.println("==========================================");
    
    #ifdef ESP32
    Serial.println("\n[1] ESP32 WiFi Setup (Bruce Style)");
    
    // Step 1: Force WiFi OFF first
    Serial.println("[2] WiFi.mode(WIFI_OFF)");
    WiFi.mode(WIFI_OFF);
    delay(200);
    
    // Step 2: Set max TX power (BRUCE STYLE - THIS IS KEY!)
    Serial.println("[3] Setting max TX power...");
    esp_wifi_set_max_tx_power(80);  // 80 = 20dBm max power
    delay(100);
    
    // Step 3: Set WiFi mode to AP
    Serial.println("[4] WiFi.mode(WIFI_AP)");
    WiFi.mode(WIFI_AP);
    delay(200);
    
    // Step 4: Configure softAP IP (BEFORE softAP!)
    Serial.println("[5] WiFi.softAPConfig()");
    WiFi.softAPConfig(ap_ip, ap_gateway, ap_subnet);
    delay(100);
    
    // Step 5: Start softAP
    Serial.println("[6] WiFi.softAP()");
    Serial.print("    SSID: ");
    Serial.println(ap_ssid);
    Serial.print("    Password: ");
    Serial.println(ap_password);
    
    bool ap_result = WiFi.softAP(ap_ssid, ap_password);
    Serial.print("    Result: ");
    Serial.println(ap_result ? "SUCCESS" : "FAILED");
    
    delay(500);
    
    // Step 6: Verify
    Serial.print("[7] AP IP: ");
    Serial.println(WiFi.softAPIP());
    
    Serial.print("[8] AP MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    
    Serial.print("[9] TX Power: ");
    int8_t power;
    esp_wifi_get_max_tx_power(&power);
    Serial.println(power);
    
    #else
    // ESP8266
    WiFi.softAPConfig(ap_ip, ap_gateway, ap_subnet);
    WiFi.softAP(ap_ssid, ap_password);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    #endif
    
    // DNS Server
    Serial.println("[10] Starting DNS...");
    dnsServer.start(53, "*", ap_ip);
    
    // Web Server
    Serial.println("[11] Starting Web Server...");
    server.on("/", [](){
        String html = "<html><head>";
        html += "<title>ESP32 Deauther V3</title>";
        html += "<style>";
        html += "body{font-family:Arial;padding:20px;background:#1a1a2e;color:#fff;}";
        html += "h1{color:#667eea;}";
        html += ".card{background:#16213e;padding:20px;border-radius:10px;margin:10px 0;}";
        html += "</style></head><body>";
        html += "<h1>ESP8266 DEAUTHER V3</h1>";
        html += "<div class='card'>";
        html += "<p><strong>Status:</strong> WORKING!</p>";
        html += "<p><strong>SSID:</strong> " + String(ap_ssid) + "</p>";
        html += "<p><strong>Password:</strong> " + String(ap_password) + "</p>";
        html += "<p><strong>AP IP:</strong> 192.168.4.1</p>";
        html += "</div>";
        html += "</body></html>";
        server.send(200, "text/html", html);
    });
    server.begin();
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("  SETUP COMPLETE!");
    Serial.println("  Search WiFi 'GMpro' on your device!");
    Serial.println("==========================================");
}

void loop() {
    #ifdef ESP32
    dnsServer.processNextRequest();
    #else
    dnsServer.processNextRequest();
    #endif
    server.handleClient();
    
    static uint32_t last = 0;
    if (millis() - last > 5000) {
        last = millis();
        #ifdef ESP32
        Serial.printf("[Status] Stations: %d\n", WiFi.softAPgetStationNum());
        #endif
    }
}
