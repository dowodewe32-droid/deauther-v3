#include <stdio.h>
#include <string.h>
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "htool_web.h"
#include "htool_api.h"
#include "htool_wifi.h"

#define AP_SSID "GMpro"
#define AP_PASS "Sangkur87"

static httpd_handle_t server = NULL;

static bool web_deauther_enabled = false;
static bool web_beacon_enabled = false;
static bool web_evil_twin_enabled = false;
static bool web_captive_portal_enabled = false;

static const char INDEX_HTML[] = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Hacking Tool</title>
    <style>
        body { font-family: Arial; max-width: 800px; margin: 0 auto; padding: 20px; background: #1a1a1a; color: #fff; }
        h1 { color: #00ff00; }
        .btn { display: inline-block; padding: 10px 20px; margin: 5px; background: #00aa00; color: white; text-decoration: none; border-radius: 5px; border: none; cursor: pointer; }
        .btn:hover { background: #00cc00; }
        .btn-red { background: #cc0000; }
        .btn-red:hover { background: #ee0000; }
        .btn-blue { background: #0066cc; }
        .btn-blue:hover { background: #0088ee; }
        .status { margin: 10px 0; padding: 10px; background: #333; border-radius: 5px; }
        table { width: 100%; border-collapse: collapse; margin: 10px 0; }
        th, td { padding: 8px; text-align: left; border-bottom: 1px solid #444; }
        th { background: #333; }
    </style>
</head>
<body>
    <h1>ESP32 Hacking Tool</h1>
    <div class="status">
        <strong>Deauther:</strong> <span id="deauth">OFF</span><br>
        <strong>Beacon:</strong> <span id="beacon">OFF</span><br>
        <strong>Evil Twin:</strong> <span id="eviltwin">OFF</span>
    </div>
    <h2>Control</h2>
    <a href="/scan" class="btn btn-blue">[Scan WiFi]</a>
    <a href="/deauth" class="btn">[Start Deauth]</a>
    <a href="/stopdeauth" class="btn btn-red">[Stop Deauth]</a><br>
    <a href="/beacon" class="btn">[Start Beacon]</a>
    <a href="/stopbeacon" class="btn btn-red">[Stop Beacon]</a><br>
    <a href="/stop" class="btn btn-red">[Stop All]</a>
    <h2>Networks</h2>
    <table>
        <tr><th>#</th><th>SSID</th><th>RSSI</th><th>Channel</th></tr>
        <tr id="networks"><td colspan="4">Click Scan to search</td></tr>
    </table>
    <script>
        function updateStatus() {
            fetch('/status').then(r=>r.json()).then(d=>{
                document.getElementById('deauth').innerText = d.deauth ? 'ON' : 'OFF';
                document.getElementById('beacon').innerText = d.beacon ? 'ON' : 'OFF';
                document.getElementById('eviltwin').innerText = d.eviltwin ? 'ON' : 'OFF';
            });
        }
        setInterval(updateStatus, 2000);
    </script>
</body>
</html>
)";

static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t scan_handler(httpd_req_t *req) {
    htool_api_start_active_scan();
    vTaskDelay(pdMS_TO_TICKS(3000));
    httpd_resp_send(req, "Scanning...", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t deauth_handler(httpd_req_t *req) {
    htool_api_start_deauther();
    httpd_resp_send(req, "Deauth Started", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t stopdeauth_handler(httpd_req_t *req) {
    htool_api_stop_deauther();
    httpd_resp_send(req, "Deauth Stopped", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t beacon_handler(httpd_req_t *req) {
    htool_api_start_beacon_spammer(0);
    httpd_resp_send(req, "Beacon Started", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t stopbeacon_handler(httpd_req_t *req) {
    htool_api_stop_beacon_spammer();
    httpd_resp_send(req, "Beacon Stopped", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t stopall_handler(httpd_req_t *req) {
    htool_api_stop_deauther();
    htool_api_stop_beacon_spammer();
    htool_api_stop_evil_twin();
    htool_api_stop_captive_portal();
    httpd_resp_send(req, "All Stopped", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t status_handler(httpd_req_t *req) {
    char json[200];
    int len = snprintf(json, sizeof(json),
        "{\"deauth\":%d,\"beacon\":%d,\"eviltwin\":%d}",
        htool_api_is_deauther_running(),
        htool_api_is_beacon_spammer_running(),
        htool_api_is_evil_twin_running());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, len);
    return ESP_OK;
}

void htool_web_init() {
    // Start WiFi AP first
    wifi_config_t wifi_config = {0};
    strcpy((char *)wifi_config.ap.ssid, AP_SSID);
    strcpy((char *)wifi_config.ap.password, AP_PASS);
    wifi_config.ap.ssid_len = strlen(AP_SSID);
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    wifi_config.ap.max_connection = 4;
    
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
    
    // Start HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri = {.uri = "/", .method = HTTP_GET, .handler = root_handler};
        httpd_register_uri_handler(server, &uri);
        
        uri.uri = "/scan"; uri.handler = scan_handler;
        httpd_register_uri_handler(server, &uri);
        
        uri.uri = "/deauth"; uri.handler = deauth_handler;
        httpd_register_uri_handler(server, &uri);
        
        uri.uri = "/stopdeauth"; uri.handler = stopdeauth_handler;
        httpd_register_uri_handler(server, &uri);
        
        uri.uri = "/beacon"; uri.handler = beacon_handler;
        httpd_register_uri_handler(server, &uri);
        
        uri.uri = "/stopbeacon"; uri.handler = stopbeacon_handler;
        httpd_register_uri_handler(server, &uri);
        
        uri.uri = "/stop"; uri.handler = stopall_handler;
        httpd_register_uri_handler(server, &uri);
        
        uri.uri = "/status"; uri.handler = status_handler;
        httpd_register_uri_handler(server, &uri);
        
        printf("Web UI started on port 80\n");
    }
}

void htool_web_handle_client() {
    // Handled by httpd
}

bool htool_web_is_deauther_enabled() { return htool_api_is_deauther_running(); }
bool htool_web_is_beacon_enabled() { return htool_api_is_beacon_spammer_running(); }
bool htool_web_is_evil_twin_enabled() { return htool_api_is_evil_twin_running(); }
bool htool_web_is_captive_portal_enabled() { return htool_api_is_captive_portal_running(); }

void htool_web_start_deauther() { htool_api_start_deauther(); }
void htool_web_stop_deauther() { htool_api_stop_deauther(); }
void htool_web_start_beacon(uint8_t num) { htool_api_start_beacon_spammer(num); }
void htool_web_stop_beacon() { htool_api_stop_beacon_spammer(); }
void htool_web_start_evil_twin(uint8_t ssid_index) { htool_api_start_evil_twin(ssid_index, 0); }
void htool_web_stop_evil_twin() { htool_api_stop_evil_twin(); }
void htool_web_start_captive_portal(uint8_t cp_index) { htool_api_start_captive_portal(cp_index); }
void htool_web_stop_captive_portal() { htool_api_stop_captive_portal(); }

int htool_web_get_scan_count() { return global_scans_count; }
void htool_web_get_scan_result(int index, char* ssid, int8_t* rssi, uint8_t* channel) {
    if (index < global_scans_count) {
        strcpy(ssid, (char*)global_scans[index].ssid);
        *rssi = global_scans[index].rssi;
        *channel = global_scans[index].primary;
    }
}