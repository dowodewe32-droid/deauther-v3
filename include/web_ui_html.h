#ifndef WEB_UI_HTML_H
#define WEB_UI_HTML_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"===HTMLSTART===(
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
)===HTMLEND===";

#endif