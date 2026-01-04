#include "WiFiManager.h"
#include <LittleFS.h>

WiFiManager::WiFiManager() : server(80), dnsServer(), portalActive(false) {}

bool WiFiManager::connectWiFi(const WifiConfig &config, unsigned long timeoutMs) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid, config.password);
    
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > timeoutMs) {
            Serial.println(F("WiFi connection timeout"));
            return false;
        }
        delay(100);
        yield();
    }
    
    Serial.println(F("WiFi connected"));
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
    
    return true;
}

void WiFiManager::startCaptivePortal(const char* apSSID) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID);
    
    IPAddress apIP(SOFTAP_IP_ADDR);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, apIP, subnet);
    
    // Start DNS server for captive portal
    dnsServer.start(53, "*", apIP);
    
    // Setup web server routes
    server.on("/", [this]() { this->handleRoot(); });
    server.on("/save", HTTP_POST, [this]() { this->handleSave(); });
    server.onNotFound([this]() { this->handleNotFound(); });
    
    server.begin();
    portalActive = true;
    
    Serial.println(F("Captive portal started"));
    Serial.print(F("AP IP: "));
    Serial.println(WiFi.softAPIP());
}

void WiFiManager::handleClient() {
    if (portalActive) {
        dnsServer.processNextRequest();
        server.handleClient();
    }
}

void WiFiManager::handleRoot() {
    server.send(200, "text/html", getSetupHTML());
}

void WiFiManager::handleSave() {
    AppConfig config;
    
    // Get form data
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    String url = server.arg("url");
    String refresh = server.arg("refresh");
    
    // Populate config struct
    strncpy(config.wifi.ssid, ssid.c_str(), sizeof(config.wifi.ssid) - 1);
    strncpy(config.wifi.password, password.c_str(), sizeof(config.wifi.password) - 1);
    strncpy(config.server.url, url.c_str(), sizeof(config.server.url) - 1);
    config.refresh_ms = refresh.toInt();
    
    // Clamp refresh rate
    if (config.refresh_ms < MIN_REFRESH_MS) config.refresh_ms = MIN_REFRESH_MS;
    if (config.refresh_ms > MAX_REFRESH_MS) config.refresh_ms = MAX_REFRESH_MS;
    
    // Validate and save
    if (validateConfig(config) && saveConfig(config)) {
        String html = F("<!DOCTYPE html><html><head><title>Saved</title>");
        html += F("<meta http-equiv='refresh' content='3;url=/'></head>");
        html += F("<body><h1>Configuration Saved!</h1>");
        html += F("<p>Device will reboot in 3 seconds...</p></body></html>");
        
        server.send(200, "text/html", html);
        
        delay(500);
        ESP.restart();
    } else {
        String html = F("<!DOCTYPE html><html><head><title>Error</title></head>");
        html += F("<body><h1>Invalid Configuration</h1>");
        html += F("<p><a href='/'>Go back</a></p></body></html>");
        
        server.send(400, "text/html", html);
    }
}

void WiFiManager::handleNotFound() {
    // Redirect to root for captive portal
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
}

String WiFiManager::getSetupHTML() {
    String html = F("<!DOCTYPE html><html><head><title>ARB Dashboard Setup</title>");
    html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    html += F("<style>");
    html += F("body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}");
    html += F(".container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}");
    html += F("h1{color:#333;text-align:center}");
    html += F("label{display:block;margin-top:15px;color:#555;font-weight:bold}");
    html += F("input,select{width:100%;padding:8px;margin-top:5px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box}");
    html += F("button{width:100%;margin-top:20px;padding:12px;background:#007bff;color:white;border:none;border-radius:4px;font-size:16px;cursor:pointer}");
    html += F("button:hover{background:#0056b3}");
    html += F(".info{background:#e7f3ff;padding:10px;border-radius:4px;margin-top:10px;font-size:14px}");
    html += F("</style></head><body>");
    html += F("<div class='container'>");
    html += F("<h1>📊 ARB Dashboard</h1>");
    html += F("<div class='info'>Configure your WiFi and server settings</div>");
    html += F("<form method='POST' action='/save'>");
    
    html += F("<label>WiFi SSID:</label>");
    html += F("<input type='text' name='ssid' required maxlength='32' placeholder='MyWiFi'>");
    
    html += F("<label>WiFi Password:</label>");
    html += F("<input type='password' name='password' maxlength='64' placeholder='Password (optional)'>");
    
    html += F("<label>Server URL:</label>");
    html += F("<input type='text' name='url' required value='");
    html += DEFAULT_SERVER_URL;
    html += F("' placeholder='http://192.168.1.10:8080'>");
    
    html += F("<label>Refresh Interval (ms):</label>");
    html += F("<input type='number' name='refresh' min='1000' max='15000' value='3000'>");
    
    html += F("<button type='submit'>Save & Reboot</button>");
    html += F("</form></div></body></html>");
    
    return html;
}
