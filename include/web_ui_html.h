#ifndef WEB_UI_HTML_H
#define WEB_UI_HTML_H

const char INDEX_HTML[] PROGMEM = 
"<html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>GMpro87dev</title>"
"<style>*{box-sizing:border-box;margin:0;padding:0}body{font-family:-apple-system,sans-serif;background:#0f172a;color:#fff;padding:20px}"
".header{background:#1e293b;padding:20px;border-radius:12px;margin-bottom:20px;text-align:center}.header h1{color:#818cf8;font-size:2em}"
".nav{display:flex;gap:10px;margin-bottom:20px;overflow-x:auto}.nav button{flex:1;padding:12px;border:none;border-radius:8px;background:#334155;color:#94a3b8;font-weight:600;cursor:pointer}"
".nav button.active{background:#6366f1;color:#fff}.card{background:#1e293b;border-radius:12px;padding:20px;margin-bottom:15px}.card h2{color:#a78bfa;margin-bottom:15px}"
".btn{padding:12px 24px;border:none;border-radius:8px;cursor:pointer;margin:5px;font-weight:600}.btn-primary{background:#6366f1;color:#fff}"
".btn-danger{background:#ef4444;color:#fff}.btn:hover{opacity:0.9}.status{background:#64748b;padding:5px 15px;border-radius:20px;font-size:12px}"
".status-on{background:#22c55e}.info{background:#334155;padding:10px;border-radius:8px;margin-top:10px}.input{width:100%;padding:12px;margin:5px 0;border:none;border-radius:8px;background:#0f172a;color:#fff}"
".section{display:none}.section.show{display:block}</style></head>"
"<body>"
"<div class=\"header\"><h1>GMpro87dev</h1><p>Full Features: Deauth + Beacon + Evil Twin + BLE + Whitelist</p></div>"
"<div class=\"nav\"><button class=\"active\" onclick=\"showTab('scan')\">Scanner</button><button onclick=\"showTab('attack')\">Attack</button>"
"<button onclick=\"showTab('beacon')\">Beacon</button><button onclick=\"showTab('etwin')\">Evil Twin</button>"
"<button onclick=\"showTab('ble')\">BLE</button><button onclick=\"showTab('wlist')\">Whitelist</button></div>"

"<div id=\"scan\" class=\"section show\"><div class=\"card\"><h2>WiFi Scanner</h2><button class=\"btn btn-primary\" onclick=\"scanWiFi()\">Scan</button>"
"<div id=\"scanResult\" class=\"info\">Tap scan</div></div></div>"

"<div id=\"attack\" class=\"section\"><div class=\"card\"><h2>Deauth Attack</h2><p>Sent: <span id=\"deauthCount\">0</span></p>"
"<button class=\"btn btn-danger\" onclick=\"startDeauth()\">Start Deauth All</button><button class=\"btn btn-primary\" onclick=\"stopAttack()\">Stop</button></div></div>"

"<div id=\"beacon\" class=\"section\"><div class=\"card\"><h2>Beacon Spam</h2><p>Status: <span id=\"beaconStatus\" class=\"status\">OFF</span> | Frames: <span id=\"beaconCount\">0</span></p>"
"<button class=\"btn btn-primary\" onclick=\"startBeacon()\">Start</button><button class=\"btn btn-danger\" onclick=\"stopBeacon()\">Stop</button></div></div>"

"<div id=\"etwin\" class=\"section\"><div class=\"card\"><h2>Evil Twin</h2><p>Status: <span id=\"etStatus\" class=\"status\">OFF</span> | Clients: <span id=\"etClients\">0</span></p>"
"<input id=\"targetSsid\" class=\"input\" placeholder=\"Target SSID\">"
"<button class=\"btn btn-primary\" onclick=\"startET()\">Start AP</button><button class=\"btn btn-danger\" onclick=\"stopET()\">Stop</button>"
"<div id=\"etCaptured\" class=\"info\">Credentials will appear here</div></div></div>"

"<div id=\"ble\" class=\"section\"><div class=\"card\"><h2>BLE Scanner</h2><p>Status: <span id=\"bleStatus\" class=\"status\">OFF</span> | Devices: <span id=\"bleCount\">0</span></p>"
"<button class=\"btn btn-primary\" onclick=\"startBLE()\">Scan</button><button class=\"btn btn-danger\" onclick=\"stopBLE()\">Stop</button>"
"<div id=\"bleResult\" class=\"info\">No devices found</div></div></div>"

"<div id=\"wlist\" class=\"section\"><div class=\"card\"><h2>Whitelist Protection</h2><p>Protected: <span id=\"wlCount\">0</span></p>"
"<input id=\"wlMac\" class=\"input\" placeholder=\"MAC Address (AA:BB:CC:DD:EE:FF)\">"
"<input id=\"wlSSID\" class=\"input\" placeholder=\"SSID\">"
"<button class=\"btn btn-primary\" onclick=\"addWL()\">Add</button><button class=\"btn btn-danger\" onclick=\"clearWL()\">Clear All</button>"
"<div id=\"wlResult\" class=\"info\">Whitelist empty</div></div></div>"

"<script>"
"function showTab(id){document.querySelectorAll('.section').forEach(e=>e.classList.remove('show'));document.querySelectorAll('.nav button').forEach(e=>e.classList.remove('active'));"
"document.getElementById(id).classList.add('show');event.target.classList.add('active')}"
"function scanWiFi(){fetch('/scan').then(r=>r.json()).then(d=>{"
"let t='<table><tr><th>#</th><th>SSID</th><th>Ch</th><th>RSSI</th></tr>';"
"d.forEach((n,i)=>t+='<tr><td>'+i+'</td><td>'+n.ssid+'</td><td>'+n.ch+'</td><td>'+n.rssi+'</td></tr>');"
"document.getElementById('scanResult').innerHTML=d.length+' networks'+t;"
"})}"
"function startDeauth(){fetch('/deauth?type=all')}"
"function stopAttack(){fetch('/stop').then(()=>location.reload())}"
"function startBeacon(){fetch('/beacon?cmd=start');document.getElementById('beaconStatus').className='status status-on';document.getElementById('beaconStatus').innerText='ON';setInterval(updateBeacon,1000)}"
"function stopBeacon(){fetch('/beacon?cmd=stop');document.getElementById('beaconStatus').className='status';document.getElementById('beaconStatus').innerText='OFF'}"
"function updateBeacon(){fetch('/beaconStatus').then(r=>r.json()).then(d=>document.getElementById('beaconCount').innerText=d.count)}"
"function startET(){let s=document.getElementById('targetSsid').value;if(s)fetch('/etwin?ssid='+s).then(r=>r.text()).then(d=>{document.getElementById('etStatus').className='status status-on';document.getElementById('etStatus').innerText='ON'})}"
"function stopET(){fetch('/etwin?cmd=stop');document.getElementById('etStatus').className='status';document.getElementById('etStatus').innerText='OFF'}"
"function startBLE(){fetch('/ble?cmd=start').then(()=>{document.getElementById('bleStatus').className='status status-on';document.getElementById('bleStatus').innerText='ON'})}"
"function stopBLE(){fetch('/ble?cmd=stop');document.getElementById('bleStatus').className='status';document.getElementById('bleStatus').innerText='OFF'}"
"function addWL(){let m=document.getElementById('wlMac').value;let s=document.getElementById('wlSSID').value;if(m&&s)fetch('/wl_add?mac='+m+'&ssid='+s).then(()=>location.reload())}"
"function clearWL(){fetch('/wl_clear').then(()=>location.reload())}"
"setInterval(()=>fetch('/status').then(r=>r.json()).then(d=>document.getElementById('deauthCount').innerText=d.deauth),2000);"
"</script></body></html>";

#endif