// ESP32 WiFi AP ONLY - NO WEB SERVER, NO DNS
// Pure WiFi test only

#include <WiFi.h>

const char* ssid = "GMpro";
const char* password = "Sangkur87";

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== ESP32 WiFi AP TEST ===\n");
    
    Serial.println("1. WiFi.mode(WIFI_AP)");
    WiFi.mode(WIFI_AP);
    
    Serial.println("2. WiFi.softAP()");
    Serial.print("   SSID: ");
    Serial.println(ssid);
    Serial.print("   PASS: ");
    Serial.println(password);
    WiFi.softAP(ssid, password);
    
    delay(500);
    
    Serial.print("3. AP IP: ");
    Serial.println(WiFi.softAPIP());
    
    Serial.print("4. AP MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    
    Serial.print("5. Status: ");
    Serial.println(WiFi.status() == WL_AP_STARTED ? "AP STARTED!" : "FAILED");
    
    Serial.println("\n=== DONE - Search WiFi 'GMpro' ===");
}

void loop() {
    static uint32_t t = 0;
    if (millis() - t > 3000) {
        t = millis();
        Serial.printf("Stations: %d\n", WiFi.softAPgetStationNum());
    }
}
