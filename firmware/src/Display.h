#ifndef DISPLAY_H
#define DISPLAY_H

#include <TFT_eSPI.h>

enum ScreenType {
    SCREEN_BOOT,
    SCREEN_WIFI_CONNECTING,
    SCREEN_WIFI_CONNECTED,
    SCREEN_WIFI_SETUP_MODE,
    SCREEN_STATUS,
    SCREEN_ARB,
    SCREEN_PNL,
    SCREEN_ALERT
};

struct MetricsData {
    int status;          // s: 1=ok, 0=down
    int latency;         // l: ms
    int activeTriangles; // a: count
    int bestArb;         // b: percentage × 100
    int pnl;             // p: cents
    int errors;          // e: count
    unsigned long timestamp; // ts: epoch
};

class Display {
public:
    Display();
    void begin();
    
    // Boot and WiFi status screens
    void showBoot();
    void showWiFiConnecting(const char* ssid);
    void showWiFiConnected(const char* ssid, const char* ip);
    void showWiFiSetupMode(const char* apName);
    
    // Dashboard screens
    void showStatus(const MetricsData &data, int rssi);
    void showArb(const MetricsData &data);
    void showPNL(const MetricsData &data);
    
    // Alert screen
    void showAlert(const char* message);
    
    // Utility
    void clear();

private:
    TFT_eSPI tft;
    
    void drawCentered(const char* text, int y, uint16_t color, uint8_t font);
    void formatPNL(int cents, char* buffer, size_t bufSize);
    void formatPercent(int value, char* buffer, size_t bufSize);
};

#endif // DISPLAY_H
