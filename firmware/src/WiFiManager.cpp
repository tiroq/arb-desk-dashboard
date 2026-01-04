#include "WiFiManager.h"
#include <LittleFS.h>

WiFiManager::WiFiManager() : server(80), dnsServer(), portalActive(false) {}

bool WiFiManager::connectWiFi(const WifiConfig &config, unsigned long timeoutMs) {
    WiFi.disconnect(true);  // Clear previous WiFi configuration
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
    WiFi.disconnect();  // Disconnect from any station mode connections
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
    
    // Store String objects first to avoid dangling pointers
    String ssidStr = server.arg("ssid");
    String passStr = server.arg("password");
    String urlStr = server.arg("url");
    String refreshStr = server.arg("refresh");
    
    // Populate config struct from stored Strings
    strncpy(config.wifi.ssid, ssidStr.c_str(), sizeof(config.wifi.ssid) - 1);
    config.wifi.ssid[sizeof(config.wifi.ssid) - 1] = '\0';
    
    strncpy(config.wifi.password, passStr.c_str(), sizeof(config.wifi.password) - 1);
    config.wifi.password[sizeof(config.wifi.password) - 1] = '\0';
    
    strncpy(config.server.url, urlStr.c_str(), sizeof(config.server.url) - 1);
    config.server.url[sizeof(config.server.url) - 1] = '\0';
    
    config.refresh_ms = refreshStr.toInt();
    
    // Clamp refresh rate
    if (config.refresh_ms < MIN_REFRESH_MS) config.refresh_ms = MIN_REFRESH_MS;
    if (config.refresh_ms > MAX_REFRESH_MS) config.refresh_ms = MAX_REFRESH_MS;
    
    // Validate and save
    if (validateConfig(config) && saveConfig(config)) {
        // Success response - use PROGMEM string
        server.send_P(200, "text/html", 
            PSTR("<!DOCTYPE html><html><head><title>Saved</title>"
                 "<meta http-equiv='refresh' content='3;url=/'></head>"
                 "<body><h1>Configuration Saved!</h1>"
                 "<p>Device will reboot in 3 seconds...</p></body></html>"));
        
        delay(500);
        
        // Gracefully shut down captive portal services before restart
        portalActive = false;
        dnsServer.stop();
        server.stop();
        
        ESP.restart();
    } else {
        // Error response - use PROGMEM string
        server.send_P(400, "text/html",
            PSTR("<!DOCTYPE html><html><head><title>Error</title></head>"
                 "<body><h1>Invalid Configuration</h1>"
                 "<p><a href='/'>Go back</a></p></body></html>"));
    }
}
}

void WiFiManager::handleNotFound() {
    // Redirect to root for captive portal
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
}

String WiFiManager::getSetupHTML() {
    // Use String reserve to pre-allocate memory and reduce fragmentation
    String html;
    html.reserve(1500);
    
    html = F("<!DOCTYPE html><html><head><title>ARB Dashboard Setup</title>"
             "<meta name='viewport' content='width=device-width,initial-scale=1'>"
             "<style>"
             "body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}"
             ".container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}"
             "h1{color:#333;text-align:center}"
             "label{display:block;margin-top:15px;color:#555;font-weight:bold}"
             "input,select{width:100%;padding:8px;margin-top:5px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box}"
             "button{width:100%;margin-top:20px;padding:12px;background:#007bff;color:white;border:none;border-radius:4px;font-size:16px;cursor:pointer}"
             "button:hover{background:#0056b3}"
             ".info{background:#e7f3ff;padding:10px;border-radius:4px;margin-top:10px;font-size:14px}"
             "</style></head><body>"
             "<div class='container'>"
             "<h1>📊 ARB Dashboard</h1>"
             "<div class='info'>Configure your WiFi and server settings</div>"
             "<form method='POST' action='/save'>"
             "<label>WiFi SSID:</label>"
             "<input type='text' name='ssid' required maxlength='32' placeholder='MyWiFi'>"
             "<label>WiFi Password:</label>"
             "<input type='password' name='password' maxlength='64' placeholder='Password (optional)'>"
             "<label>Server URL:</label>"
             "<input type='text' name='url' required value='");
    html += DEFAULT_SERVER_URL;
    html += F("' placeholder='http://192.168.1.10:8080'>"
              "<label>Refresh Interval (ms):</label>"
              "<input type='number' name='refresh' min='1000' max='15000' value='3000'>"
              "<button type='submit'>Save & Reboot</button>"
              "</form></div></body></html>");
    
    return html;
}
