#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

const char* ssid = "GMpro";
const char* password = "Sangkur87";

WebServer server(80);
DNSServer dnsServer;

IPAddress localIP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== ESP32 AP TEST ===");
    
    // Test 1: Check WiFi mode
    Serial.print("WiFi mode: ");
    Serial.println(WiFi.getMode());
    
    // Test 2: Set mode to AP
    Serial.println("Setting mode to WIFI_AP...");
    WiFi.mode(WIFI_AP);
    Serial.print("WiFi mode after: ");
    Serial.println(WiFi.getMode());
    
    // Test 3: Configure softAP (Marauder order - config FIRST)
    Serial.println("Configuring softAP...");
    WiFi.softAPConfig(localIP, gateway, subnet);
    
    // Test 4: Start softAP
    Serial.print("Starting softAP with SSID: ");
    Serial.print(ssid);
    Serial.print(" PASS: ");
    Serial.println(password);
    
    bool result = WiFi.softAP(ssid, password);
    Serial.print("softAP result: ");
    Serial.println(result ? "SUCCESS" : "FAILED");
    
    // Test 5: Check IP
    delay(500);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    
    Serial.print("AP MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    
    Serial.print("Number of connected stations: ");
    Serial.println(WiFi.softAPgetStationNum());
    
    // Test 6: Start DNS
    dnsServer.start(53, "*", localIP);
    Serial.println("DNS started");
    
    // Test 7: Simple web server
    server.on("/", [](){
        server.send(200, "text/html", "<h1>ESP32 AP WORKS!</h1><p>GMpro is broadcasting!</p>");
    });
    server.begin();
    Serial.println("Web server started");
    
    Serial.println();
    Serial.println("=== SETUP COMPLETE ===");
    Serial.println("Search for WiFi 'GMpro' on your device!");
}

void loop() {
    dnsServer.processNextRequest();
    server.handleClient();
    
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 5000) {
        lastPrint = millis();
        Serial.print("Stations connected: ");
        Serial.println(WiFi.softAPgetStationNum());
    }
}
