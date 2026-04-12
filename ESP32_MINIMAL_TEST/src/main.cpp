/* =====================
   ESP32 DEAUTHER V3 - MINIMAL TEST
   Just tests if AP works
   ===================== */

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <esp_wifi.h>

const char* ap_ssid = "GMpro";
const char* ap_password = "Sangkur87";

const IPAddress ap_ip(192, 168, 4, 1);
const IPAddress ap_gateway(192, 168, 4, 1);
const IPAddress ap_subnet(255, 255, 255, 0);

WebServer server(80);
DNSServer dnsServer;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("==========================================");
    Serial.println("  ESP32 DEAUTHER V3 - MINIMAL TEST");
    Serial.println("==========================================");
    
    Serial.println("\n[1] WiFi.mode(WIFI_OFF)");
    WiFi.mode(WIFI_OFF);
    delay(200);
    
    Serial.println("[2] esp_wifi_set_max_tx_power(80)");
    esp_wifi_set_max_tx_power(80);
    delay(100);
    
    Serial.println("[3] WiFi.mode(WIFI_AP)");
    WiFi.mode(WIFI_AP);
    delay(200);
    
    Serial.println("[4] WiFi.softAPConfig()");
    WiFi.softAPConfig(ap_ip, ap_gateway, ap_subnet);
    delay(100);
    
    Serial.println("[5] WiFi.softAP()");
    Serial.print("    SSID: ");
    Serial.println(ap_ssid);
    Serial.print("    PASS: ");
    Serial.println(ap_password);
    
    bool result = WiFi.softAP(ap_ssid, ap_password);
    Serial.print("    Result: ");
    Serial.println(result ? "SUCCESS" : "FAILED");
    
    delay(500);
    
    Serial.print("[6] AP IP: ");
    Serial.println(WiFi.softAPIP());
    
    Serial.print("[7] AP MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    
    Serial.println("[8] DNS Server starting...");
    dnsServer.start(53, "*", ap_ip);
    
    Serial.println("[9] Web Server starting...");
    server.on("/", [](){
        server.send(200, "text/html",
            "<html><head><style>"
            "body{font-family:Arial;padding:20px;background:#1a1a2e;color:#fff;}"
            "h1{color:#667eea;}"
            ".card{background:#16213e;padding:20px;border-radius:10px;}"
            "</style></head><body>"
            "<h1>ESP32 DEAUTHER V3</h1>"
            "<div class='card'>"
            "<p><strong>Status:</strong> WORKING!</p>"
            "<p><strong>SSID:</strong> " + String(ap_ssid) + "</p>"
            "<p><strong>PASS:</strong> " + String(ap_password) + "</p>"
            "<p><strong>IP:</strong> 192.168.4.1</p>"
            "</div></body></html>");
    });
    server.begin();
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("  SETUP COMPLETE!");
    Serial.println("  Search WiFi 'GMpro' now!");
    Serial.println("==========================================");
}

void loop() {
    dnsServer.processNextRequest();
    server.handleClient();
    
    static uint32_t last = 0;
    if (millis() - last > 5000) {
        last = millis();
        Serial.printf("[Status] Stations: %d\n", WiFi.softAPgetStationNum());
    }
}
