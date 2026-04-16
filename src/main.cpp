// Simple test firmware - just starts AP with GMpro/Sangkur87
// Based on TESA original style

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

const char* ssid = "GMpro";
const char* password = "Sangkur87";

WebServer server(80);
DNSServer dnsServer;

void handleRoot() {
  server.send(200, "text/html", 
    "<html><body>"
    "<h1>GMpro87dev TEST</h1>"
    "<p>AP is working!</p>"
    "<p>If you see this, the firmware is working correctly.</p>"
    "</body></html>");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("========================================");
  Serial.println("GMpro87dev Simple Test Firmware");
  Serial.println("========================================");
  
  Serial.print("Starting AP: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_AP);
  delay(100);
  
  bool result = WiFi.softAP(ssid, password);
  
  Serial.print("softAP result: ");
  Serial.println(result ? "SUCCESS" : "FAILED");
  
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  server.on("/", handleRoot);
  server.begin();
  
  Serial.println("Web server started!");
  Serial.println("========================================");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
