// ESP32 ONLY TEST - No extras, just WiFi AP

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

// Simple AP settings
const char* ssid = "GMpro";
const char* pass = "Sangkur87";

IPAddress localIP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer webServer(80);
DNSServer dnsServer;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== ESP32 WiFi AP Test ===\n");
  
  // 1. Turn off WiFi
  Serial.println("1. WiFi OFF");
  WiFi.mode(WIFI_OFF);
  delay(100);
  
  // 2. Set mode to AP
  Serial.println("2. Set mode WIFI_AP");
  WiFi.mode(WIFI_AP);
  delay(200);
  
  // 3. Configure IP
  Serial.println("3. softAPConfig");
  WiFi.softAPConfig(localIP, gateway, subnet);
  delay(100);
  
  // 4. Start AP
  Serial.print("4. softAP: ");
  Serial.print(ssid);
  Serial.print(" / ");
  Serial.println(pass);
  WiFi.softAP(ssid, pass);
  delay(300);
  
  // 5. Check result
  Serial.print("5. softAPIP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("6. softAPmacAddress: ");
  Serial.println(WiFi.softAPmacAddress());
  Serial.print("7. stations: ");
  Serial.println(WiFi.softAPgetStationNum());
  
  // 8. Start DNS
  Serial.println("8. DNS start");
  dnsServer.start(53, "*", localIP);
  
  // 9. Web server
  Serial.println("9. WebServer start");
  webServer.on("/", [](){
    webServer.send(200, "text/html", 
      "<html><body style='font-family:Arial;background:#1a1a2e;color:#fff;padding:20px'>"
      "<h1 style='color:#667eea'>ESP32 WiFi AP WORKS!</h1>"
      "<p>If you see this, the AP is broadcasting.</p>"
      "<p>SSID: GMpro | Pass: Sangkur87</p>"
      "</body></html>");
  });
  webServer.begin();
  
  Serial.println("\n=== TEST COMPLETE ===");
  Serial.println("Look for WiFi 'GMpro' on your phone!");
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
}
