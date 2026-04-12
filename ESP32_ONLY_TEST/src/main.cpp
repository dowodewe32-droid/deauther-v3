// ESP32 ONLY TEST - COPY MARAUDER EXACTLY

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

// MARAUDER SETTINGS
const char* ssid = "GMpro";
const char* password = "Sangkur87";
const IPAddress AP_IP(172, 0, 0, 1);  // MARAUDER IP!

WebServer server(80);
DNSServer dnsServer;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== ESP32 AP TEST (MARAUDER STYLE) ===\n");
    
    Serial.println("Step 1: WiFi.mode(WIFI_AP)");
    WiFi.mode(WIFI_AP);
    
    Serial.println("Step 2: WiFi.softAPConfig(172.0.0.1)");
    WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
    
    Serial.println("Step 3: WiFi.softAP()");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("PASS: ");
    Serial.println(password);
    WiFi.softAP(ssid, password);
    
    delay(300);
    
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    
    Serial.print("AP MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    
    Serial.println("Step 4: DNS Server");
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    Serial.println("Step 5: Web Server");
    server.on("/", [](){
        server.send(200, "text/html", 
            "<html><body style='font-family:Arial;padding:30px;background:#1a1a2e;color:#fff'>"
            "<h1 style='color:#667eea'>ESP32 DEAUTHER V3</h1>"
            "<p>If you see this, AP is WORKING!</p>"
            "<p><strong>SSID:</strong> GMpro</p>"
            "<p><strong>Password:</strong> Sangkur87</p>"
            "<p><strong>IP:</strong> 172.0.0.1</p>"
            "</body></html>");
    });
    server.begin();
    
    Serial.println("\n=== SETUP COMPLETE ===");
    Serial.println("Search WiFi 'GMpro' now!");
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
