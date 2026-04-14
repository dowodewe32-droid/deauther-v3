#include <WebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "deauth.h"
#include "definitions.h"

WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

bool attackRunning = false;
bool beaconRunning = false;
bool evilTwinRunning = false;
bool bleRunning = false;
bool validatingPassword = false;

String currentSSID = "";
String currentPassword = "";
int attackMode = 0;
int currentChannel = 1;
int scanResults[50][4];
int scanCount = 0;

void handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>GMpro87dev</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --primary: #6366f1;
            --primary-dark: #4f46e5;
            --danger: #ef4444;
            --danger-dark: #dc2626;
            --success: #22c55e;
            --warning: #f59e0b;
            --info: #06b6d4;
            --dark: #0f172a;
            --darker: #020617;
            --card: #1e293b;
            --border: #334155;
            --text: #f8fafc;
            --text-dim: #94a3b8;
            --gradient: linear-gradient(135deg, #6366f1 0%, #8b5cf6 100%);
        }
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body { font-family: 'Inter', sans-serif; background: var(--darker); color: var(--text); min-height: 100vh; }
        .header { background: var(--dark); padding: 16px 20px; position: sticky; top: 0; z-index: 100; border-bottom: 1px solid var(--border); }
        .logo { font-size: 20px; font-weight: 700; background: var(--gradient); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
        .container { max-width: 600px; margin: 0 auto; padding: 16px; }
        .card { background: var(--card); padding: 20px; margin-bottom: 16px; border-radius: 12px; border: 1px solid var(--border); }
        .btn { background: var(--primary); color: white; padding: 12px 24px; border: none; border-radius: 8px; cursor: pointer; margin: 4px; font-size: 14px; font-weight: 500; }
        .btn:hover { background: var(--primary-dark); }
        .btn-danger { background: var(--danger); }
        .btn-danger:hover { background: var(--danger-dark); }
        .btn-success { background: var(--success); }
        .btn-block { display: block; width: 100%; margin: 8px 0; }
        .input { width: 100%; padding: 12px; margin: 8px 0; background: var(--dark); border: 1px solid var(--border); border-radius: 8px; color: var(--text); }
        .status { display: inline-block; padding: 4px 12px; border-radius: 20px; font-size: 12px; font-weight: 600; }
        .status.idle { background: var(--dark); }
        .status.active { background: var(--danger); animation: pulse 1s infinite; }
        @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.6; } }
        table { width: 100%; border-collapse: collapse; }
        th, td { padding: 10px; text-align: left; border-bottom: 1px solid var(--border); font-size: 13px; }
        th { background: var(--dark); font-weight: 600; }
        .tab-bar { display: flex; background: var(--dark); border-radius: 8px; margin-bottom: 16px; overflow: hidden; }
        .tab { flex: 1; padding: 12px; text-align: center; cursor: pointer; font-size: 13px; font-weight: 500; }
        .tab.active { background: var(--primary); }
        .tab-content { display: none; }
        .tab-content.active { display: block; }
        .toggle { position: relative; width: 48px; height: 24px; }
        .toggle input { opacity: 0; width: 0; height: 0; }
        .slider { position: absolute; cursor: pointer; inset: 0; background: var(--border); border-radius: 24px; transition: .3s; }
        .slider:before { content: ""; position: absolute; height: 18px; width: 18px; left: 3px; bottom: 3px; background: white; border-radius: 50%; transition: .3s; }
        input:checked + .slider { background: var(--primary); }
        input:checked + .slider:before { transform: translateX(24px); }
        .toast { position: fixed; bottom: 20px; left: 50%; transform: translateX(-50%); background: var(--card); padding: 12px 24px; border-radius: 8px; border: 1px solid var(--border); z-index: 1000; animation: fadeIn 0.3s; }
        @keyframes fadeIn { from { opacity: 0; transform: translateX(-50%) translateY(20px); } }
    </style>
</head>
<body>
    <div class="header">
        <div class="header-content" style="display:flex;justify-content:space-between;align-items:center;max-width:600px;margin:0 auto;">
            <span class="logo">GMpro87dev</span>
            <span id="statusText" class="status idle">Idle</span>
        </div>
    </div>
    <div class="container">
        <div class="tab-bar">
            <div class="tab active" onclick="showTab('scan')">Scan</div>
            <div class="tab" onclick="showTab('attack')">Attack</div>
            <div class="tab" onclick="showTab('eviltwin')">Evil Twin</div>
            <div class="tab" onclick="showTab('beacon')">Beacon</div>
            <div class="tab" onclick="showTab('ble')">BLE</div>
            <div class="tab" onclick="showTab('settings')">Settings</div>
        </div>
        
        <div id="tab-scan" class="tab-content active">
            <div class="card">
                <h3 style="margin-bottom:12px;">WiFi Scan</h3>
                <button class="btn btn-block" onclick="scanWiFi()">Scan Networks</button>
                <div id="wifiResults" style="margin-top:16px;"></div>
            </div>
            <div class="card">
                <h3 style="margin-bottom:12px;">BLE Scan</h3>
                <button class="btn btn-block" onclick="scanBLE()">Scan BLE Devices</button>
                <div id="bleResults" style="margin-top:16px;"></div>
            </div>
        </div>
        
        <div id="tab-attack" class="tab-content">
            <div class="card">
                <h3 style="margin-bottom:12px;">Deauth Attack</h3>
                <p style="color:var(--text-dim);font-size:13px;margin-bottom:12px;">Select networks from scan tab first</p>
                <button class="btn btn-danger btn-block" onclick="startDeauth()">Start Deauth</button>
                <button class="btn btn-block" onclick="stopAttack()">Stop Attack</button>
            </div>
            <div class="card">
                <h3 style="margin-bottom:12px;">True Deauth (Channel Hopping)</h3>
                <p style="color:var(--text-dim);font-size:13px;margin-bottom:12px;">Attack ALL networks on all channels</p>
                <button class="btn btn-danger btn-block" onclick="startTrueDeauth()">Start True Deauth</button>
                <button class="btn btn-block" onclick="stopAttack()">Stop</button>
            </div>
        </div>
        
        <div id="tab-eviltwin" class="tab-content">
            <div class="card">
                <h3 style="margin-bottom:12px;">Evil Twin Attack</h3>
                <input type="text" id="etSSID" class="input" placeholder="SSID Name">
                <input type="text" id="etPASS" class="input" placeholder="Password (optional)">
                <button class="btn btn-danger btn-block" onclick="startEvilTwin()">Start Evil Twin</button>
                <button class="btn btn-block" onclick="stopEvilTwin()">Stop</button>
            </div>
        </div>
        
        <div id="tab-beacon" class="tab-content">
            <div class="card">
                <h3 style="margin-bottom:12px;">Beacon Spam</h3>
                <input type="text" id="beaconSSID" class="input" placeholder="SSID to broadcast">
                <button class="btn btn-block" onclick="startBeacon()">Start Beacon Spam</button>
                <button class="btn btn-block" onclick="stopBeacon()">Stop</button>
            </div>
        </div>
        
        <div id="tab-ble" class="tab-content">
            <div class="card">
                <h3 style="margin-bottom:12px;">BLE Spam</h3>
                <button class="btn btn-block" onclick="startBLESpam()">Start BLE Spam</button>
                <button class="btn btn-block" onclick="stopBLE()">Stop</button>
            </div>
        </div>
        
        <div id="tab-settings" class="tab-content">
            <div class="card">
                <h3 style="margin-bottom:12px;">Settings</h3>
                <p style="color:var(--text-dim);font-size:13px;">AP: GMpro | PASS: Sangkur87</p>
                <p style="color:var(--text-dim);font-size:13px;margin-top:8px;">Version: GMpro87dev v1.0</p>
            </div>
        </div>
    </div>
    
    <script>
        let selectedNetworks = [];
        
        function showTab(tab) {
            document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
            event.target.classList.add('active');
            document.getElementById('tab-' + tab).classList.add('active');
        }
        
        function toast(msg) {
            let t = document.createElement('div');
            t.className = 'toast';
            t.textContent = msg;
            document.body.appendChild(t);
            setTimeout(() => t.remove(), 3000);
        }
        
        async function scanWiFi() {
            toast('Scanning WiFi...');
            let res = await fetch('/scan');
            let text = await res.text();
            document.getElementById('wifiResults').innerHTML = text;
        }
        
        async function scanBLE() {
            toast('Scanning BLE...');
            let res = await fetch('/blescan');
            let text = await res.text();
            document.getElementById('bleResults').innerHTML = text;
        }
        
        async function startDeauth() {
            let chk = document.querySelectorAll('input[type=checkbox]:checked');
            let nets = [];
            chk.forEach(c => { if(c.dataset.bssid) nets.push(c.dataset.bssid); });
            if(nets.length == 0) { toast('Select networks first!'); return; }
            await fetch('/deauth?mode=single&nets=' + nets.join(','));
            toast('Deauth started!');
            document.getElementById('statusText').textContent = 'Deauth';
            document.getElementById('statusText').className = 'status active';
        }
        
        async function startTrueDeauth() {
            await fetch('/deauth?mode=all');
            toast('True Deauth started!');
            document.getElementById('statusText').textContent = 'True Deauth';
            document.getElementById('statusText').className = 'status active';
        }
        
        async function stopAttack() {
            await fetch('/stop');
            toast('Stopped');
            document.getElementById('statusText').textContent = 'Idle';
            document.getElementById('statusText').className = 'status idle';
        }
        
        async function startEvilTwin() {
            let ssid = document.getElementById('etSSID').value || 'FreeWiFi';
            let pass = document.getElementById('etPASS').value || '';
            await fetch('/eviltwin?ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass));
            toast('Evil Twin: ' + ssid);
        }
        
        async function stopEvilTwin() {
            await fetch('/eviltwin?stop=1');
            toast('Evil Twin stopped');
        }
        
        async function startBeacon() {
            let ssid = document.getElementById('beaconSSID').value || 'Test';
            await fetch('/beacon?ssid=' + encodeURIComponent(ssid));
            toast('Beacon spam: ' + ssid);
        }
        
        async function stopBeacon() {
            await fetch('/beacon?stop=1');
            toast('Beacon stopped');
        }
        
        async function startBLESpam() {
            await fetch('/blespam?start=1');
            toast('BLE Spam started');
        }
        
        async function stopBLE() {
            await fetch('/blespam?stop=1');
            toast('BLE stopped');
        }
        
        function toggle(id) {
            let el = document.getElementById(id);
            el.style.display = el.style.display === 'none' ? 'block' : 'none';
        }
    </script>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
}

void handleScan() {
    String html = "<table><tr><th>SSID</th><th>Ch</th><th>RSSI</th><th>Select</th></tr>";
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n && i < 20; i++) {
        html += "<tr><td>" + WiFi.SSID(i) + "</td><td>" + WiFi.channel(i) + "</td><td>" + WiFi.RSSI(i) + " dBm</td>";
        String bssid; for(int b=0;b<6;b++) { bssid += (b?":":"") + String(WiFi.BSSID(i)[b], HEX); } html += "<td><input type='checkbox' data-bssid='" + bssid + "'></td></tr>";
    }
    html += "</table>";
    if (n == 0) html = "<p>No networks found</p>";
    WiFi.scanDelete();
    server.send(200, "text/html", html);
}

void handleBLEScan() {
    server.send(200, "text/html", "<p>BLE Scan - Placeholder</p><p>Connect BLE module for full scan</p>");
}

void handleDeauth() {
    String mode = server.arg("mode");
    if (mode == "all") {
        attackMode = DEAUTH_TYPE_ALL;
        WiFi.softAPdisconnect();
        WiFi.mode(WIFI_MODE_STA);
    } else {
        attackMode = DEAUTH_TYPE_SINGLE;
    }
    server.send(200, "text/plain", "OK");
}

void handleStop() {
    attackRunning = false;
    beaconRunning = false;
    evilTwinRunning = false;
    stop_deauth();
    server.send(200, "text/plain", "OK");
}

void handleEvilTwin() {
    if (server.hasArg("stop")) {
        WiFi.softAPdisconnect();
        evilTwinRunning = false;
    } else {
        String ssid = server.arg("ssid");
        String pass = server.arg("pass");
        if (ssid.length() > 0) {
            if (pass.length() > 0) {
                WiFi.softAP(ssid.c_str(), pass.c_str());
            } else {
                WiFi.softAP(ssid.c_str());
            }
            evilTwinRunning = true;
        }
    }
    server.send(200, "text/plain", "OK");
}

void handleBeacon() {
    if (server.hasArg("stop")) {
        beaconRunning = false;
    } else {
        beaconRunning = true;
    }
    server.send(200, "text/plain", "OK");
}

void handleBLESpam() {
    if (server.hasArg("stop")) {
        bleRunning = false;
    } else {
        bleRunning = true;
    }
    server.send(200, "text/plain", "OK");
}

void handleCaptivePortal() {
    server.sendHeader("Location", "http://192.168.4.1/", 302);
    server.send(302);
}

void handleNotFound() {
    if (server.hostHeader() != "192.168.4.1") {
        handleCaptivePortal();
    } else {
        server.send(404, "text/plain", "Not Found");
    }
}

void start_web_interface() {
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    
    server.on("/", handleRoot);
    server.on("/scan", handleScan);
    server.on("/blescan", handleBLEScan);
    server.on("/deauth", handleDeauth);
    server.on("/stop", handleStop);
    server.on("/eviltwin", handleEvilTwin);
    server.on("/beacon", handleBeacon);
    server.on("/blespam", handleBLESpam);
    server.onNotFound(handleNotFound);
    
    server.begin();
    Serial.println("Web server started");
}
