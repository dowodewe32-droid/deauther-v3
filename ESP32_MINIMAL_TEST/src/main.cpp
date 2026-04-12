/* =====================
   ESP8266 DEAUTHER - MINIMAL TEST VERSION
   Only tests if AP works
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
    Serial.println();
    Serial.println("==========================================");
    Serial.println("  ESP8266 DEAUTHER V3 - MINIMAL TEST");
    Serial.println("==========================================");
    Serial.println();
    
    // Force settings reset
    EEPROM.begin(4096);
    EEPROM.put(0, 0xDEADBEEF);  // Magic number to trigger reset
    EEPROM.commit();
    EEPROM.end();
    
    #ifdef ESP32
    Serial.println("[1] ESP32 WiFi Init");
    WiFi.mode(WIFI_OFF);
    delay(200);
    
    Serial.println("[2] Setting WiFi Mode to WIFI_AP");
    WiFi.mode(WIFI_AP);
    delay(100);
    
    Serial.println("[3] Configuring softAP IP");
    WiFi.softAPConfig(ap_ip, ap_gateway, ap_subnet);
    delay(100);
    
    Serial.println("[4] Starting softAP...");
    Serial.print("    SSID: ");
    Serial.println(ap_ssid);
    Serial.print("    Password: ");
    Serial.println(ap_password);
    
    bool ap_result = WiFi.softAP(ap_ssid, ap_password);
    Serial.print("    Result: ");
    Serial.println(ap_result ? "SUCCESS" : "FAILED");
    
    delay(500);
    
    Serial.print("[5] AP IP: ");
    Serial.println(WiFi.softAPIP());
    
    Serial.print("[6] AP MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    
    Serial.print("[7] Stations connected: ");
    Serial.println(WiFi.softAPgetStationNum());
    
    #else
    WiFi.softAPConfig(ap_ip, ap_gateway, ap_subnet);
    WiFi.softAP(ap_ssid, ap_password);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    #endif
    
    Serial.println("[8] Starting DNS Server...");
    dnsServer.start(53, "*", ap_ip);
    
    Serial.println("[9] Starting Web Server...");
    server.on("/", [](){
        String html = "<html><head>";
        html += "<title>ESP32 Deauther</title>";
        html += "<style>";
        html += "body{font-family:Arial;padding:20px;background:#1a1a2e;color:#fff;}";
        html += "h1{color:#667eea;}";
        html += ".status{background:#16213e;padding:20px;border-radius:10px;margin:10px 0;}";
        html += ".btn{background:#667eea;color:#fff;padding:15px 30px;border:none;border-radius:5px;cursor:pointer;}";
        html += "</style></head><body>";
        html += "<h1>ESP8266 DEAUTHER V3</h1>";
        html += "<div class='status'>";
        html += "<p><strong>AP Status:</strong> WORKING</p>";
        html += "<p><strong>SSID:</strong> " + String(ap_ssid) + "</p>";
        html += "<p><strong>IP:</strong> 192.168.4.1</p>";
        html += "<p><strong>Web Interface:</strong> Ready</p>";
        html += "</div>";
        html += "<p>Firmware: MINIMAL TEST VERSION</p>";
        html += "</body></html>";
        server.send(200, "text/html", html);
    });
    server.begin();
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("  SETUP COMPLETE - AP SHOULD BE VISIBLE");
    Serial.println("==========================================");
    Serial.println();
    Serial.println("Search for WiFi 'GMpro' on your device!");
    Serial.println("Connect and open browser to 192.168.4.1");
}

void loop() {
    #ifdef ESP32
    dnsServer.processNextRequest();
    #else
    dnsServer.processNextRequest();
    #endif
    server.handleClient();
    
    // Print station count every 5 seconds
    static uint32_t last_check = 0;
    if (millis() - last_check > 5000) {
        last_check = millis();
        #ifdef ESP32
        Serial.print("[Status] Stations connected: ");
        Serial.println(WiFi.softAPgetStationNum());
        #endif
    }
}
