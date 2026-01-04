#include <Arduino.h>
#include <LittleFS.h>
#include "Config.h"
#include "Display.h"
#include "WiFiManager.h"
#include "MetricsClient.h"

// Global objects
Display display;
WiFiManager wifiManager;
MetricsClient metricsClient;
AppConfig appConfig;

// State variables
unsigned long lastMetricsFetch = 0;
unsigned long lastScreenRotation = 0;
int currentScreen = 0;
bool alertMode = false;
MetricsData currentMetrics = {0};

void setup() {
    Serial.begin(115200);
    Serial.println(F("\n\n=== ARB Desk Dashboard ==="));
    
    // Initialize display
    display.begin();
    display.showBoot();
    delay(1000);
    
    // Mount filesystem
    if (!LittleFS.begin()) {
        Serial.println(F("LittleFS mount failed!"));
        display.showAlert("FS MOUNT FAIL");
        while(1) {
            yield();
            delay(1000);
        }
    }
    
    // Load configuration
    bool configLoaded = loadConfig(appConfig);
    
    if (!configLoaded || strlen(appConfig.wifi.ssid) == 0) {
        Serial.println(F("No valid config, starting AP mode"));
        display.showWiFiSetupMode(DEFAULT_AP_SSID);
        wifiManager.startCaptivePortal(DEFAULT_AP_SSID);
        return;
    }
    
    // Attempt WiFi connection
    display.showWiFiConnecting(appConfig.wifi.ssid);
    
    bool connected = wifiManager.connectWiFi(appConfig.wifi, WIFI_CONNECT_TIMEOUT_MS);
    
    if (!connected) {
        Serial.println(F("WiFi connection failed, starting AP mode"));
        display.showWiFiSetupMode(DEFAULT_AP_SSID);
        wifiManager.startCaptivePortal(DEFAULT_AP_SSID);
        return;
    }
    
    // Show WiFi connected screen
    char ipStr[16];
    IPAddress ip = WiFi.localIP();
    snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    display.showWiFiConnected(appConfig.wifi.ssid, ipStr);
    delay(WIFI_STATUS_DISPLAY_MS);
    
    // Initialize metrics client
    metricsClient.setServerUrl(appConfig.server.url);
    
    Serial.println(F("Setup complete, entering main loop"));
}

void loop() {
    // Handle captive portal if active
    if (wifiManager.isPortalActive()) {
        wifiManager.handleClient();
        yield();
        return;
    }
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WiFi disconnected, attempting reconnect..."));
        display.showWiFiConnecting(appConfig.wifi.ssid);
        
        if (wifiManager.connectWiFi(appConfig.wifi, WIFI_CONNECT_TIMEOUT_MS)) {
            char ipStr[16];
            IPAddress ip = WiFi.localIP();
            snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
            display.showWiFiConnected(appConfig.wifi.ssid, ipStr);
            delay(WIFI_STATUS_DISPLAY_MS);
            alertMode = false;
        } else {
            display.showAlert("NO WIFI");
            delay(5000);
        }
        return;
    }
    
    // Fetch metrics at configured interval
    unsigned long now = millis();
    if (now - lastMetricsFetch >= appConfig.refresh_ms) {
        lastMetricsFetch = now;
        
        bool success = metricsClient.fetchMetrics(currentMetrics);
        
        if (!success) {
            Serial.print(F("Metrics fetch failed. Failures: "));
            Serial.println(metricsClient.getFailureCount());
        }
        
        // Check for alert conditions
        if (metricsClient.getFailureCount() >= MAX_CONSECUTIVE_FAILURES || currentMetrics.status == 0) {
            alertMode = true;
        } else {
            alertMode = false;
        }
    }
    
    // Display logic
    if (alertMode) {
        if (metricsClient.getFailureCount() >= MAX_CONSECUTIVE_FAILURES) {
            display.showAlert("NO DATA");
        } else if (currentMetrics.status == 0) {
            display.showAlert("BOT DOWN");
        }
        delay(1000);
    } else {
        // Rotate through screens
        // Note: millis() rollover (~49.7 days) is handled correctly by unsigned arithmetic
        if (now - lastScreenRotation >= SCREEN_ROTATION_MS) {
            lastScreenRotation = now;
            currentScreen = (currentScreen + 1) % 3;
        }
        
        int rssi = WiFi.RSSI();
        
        switch (currentScreen) {
            case 0:
                display.showStatus(currentMetrics, rssi);
                break;
            case 1:
                display.showArb(currentMetrics);
                break;
            case 2:
                display.showPNL(currentMetrics);
                break;
        }
    }
    
    yield();
    delay(100);
}
