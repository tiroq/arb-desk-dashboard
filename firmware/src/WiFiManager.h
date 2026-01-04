#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "Config.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

class WiFiManager {
public:
    WiFiManager();
    
    bool connectWiFi(const WifiConfig &config, unsigned long timeoutMs);
    void startCaptivePortal(const char* apSSID);
    void handleClient();
    bool isPortalActive() const { return portalActive; }
    
private:
    ESP8266WebServer server;
    DNSServer dnsServer;
    bool portalActive;
    
    void handleRoot();
    void handleSave();
    void handleNotFound();
    
    String getSetupHTML();
};

#endif // WIFI_MANAGER_H
