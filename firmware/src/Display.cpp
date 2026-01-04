#include "Display.h"
#include <Arduino.h>

Display::Display() : tft() {}

void Display::begin() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
}

void Display::clear() {
    tft.fillScreen(TFT_BLACK);
}

void Display::drawCentered(const char* text, int y, uint16_t color, uint8_t font) {
    tft.setTextColor(color);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(text, 120, y, font);
}

void Display::showBoot() {
    clear();
    drawCentered("BOOTING...", 100, TFT_WHITE, 4);
    drawCentered("Mounting FS", 140, TFT_CYAN, 2);
}

void Display::showWiFiConnecting(const char* ssid) {
    clear();
    drawCentered("Connecting to WiFi", 90, TFT_WHITE, 4);
    
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "SSID: %s", ssid);
    drawCentered(buffer, 140, TFT_CYAN, 2);
    
    // Simple animation indicator
    drawCentered("...", 180, TFT_YELLOW, 4);
}

void Display::showWiFiConnected(const char* ssid, const char* ip) {
    clear();
    drawCentered("WiFi Connected", 80, TFT_GREEN, 4);
    
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s", ssid);
    drawCentered(buffer, 130, TFT_WHITE, 2);
    
    snprintf(buffer, sizeof(buffer), "IP: %s", ip);
    drawCentered(buffer, 160, TFT_CYAN, 2);
}

void Display::showWiFiSetupMode(const char* apName) {
    clear();
    drawCentered("WiFi Setup Mode", 60, TFT_YELLOW, 4);
    
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "AP: %s", apName);
    drawCentered(buffer, 110, TFT_WHITE, 2);
    
    drawCentered("IP: 192.168.4.1", 140, TFT_CYAN, 2);
    drawCentered("Open browser", 170, TFT_GREEN, 2);
    drawCentered("to configure", 190, TFT_GREEN, 2);
}

void Display::showStatus(const MetricsData &data, int rssi) {
    clear();
    
    // Title
    drawCentered("STATUS", 30, TFT_YELLOW, 4);
    
    // Bot status
    const char* statusText = (data.status == 1) ? "OK" : "DOWN";
    uint16_t statusColor = (data.status == 1) ? TFT_GREEN : TFT_RED;
    drawCentered("Bot:", 80, TFT_WHITE, 2);
    drawCentered(statusText, 105, statusColor, 4);
    
    // WiFi RSSI
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "WiFi: %d dBm", rssi);
    drawCentered(buffer, 150, TFT_CYAN, 2);
    
    // Latency
    snprintf(buffer, sizeof(buffer), "Latency: %d ms", data.latency);
    drawCentered(buffer, 180, TFT_WHITE, 2);
}

void Display::showArb(const MetricsData &data) {
    clear();
    
    // Title
    drawCentered("ARBITRAGE", 30, TFT_YELLOW, 4);
    
    // Active triangles
    drawCentered("Active:", 80, TFT_WHITE, 2);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", data.activeTriangles);
    drawCentered(buffer, 110, TFT_GREEN, 4);
    
    // Best percentage
    drawCentered("Best %:", 160, TFT_WHITE, 2);
    formatPercent(data.bestArb, buffer, sizeof(buffer));
    drawCentered(buffer, 190, TFT_CYAN, 4);
}

void Display::showPNL(const MetricsData &data) {
    clear();
    
    // Title
    drawCentered("PNL TODAY", 30, TFT_YELLOW, 4);
    
    // PNL amount
    char buffer[32];
    formatPNL(data.pnl, buffer, sizeof(buffer));
    
    uint16_t pnlColor = (data.pnl >= 0) ? TFT_GREEN : TFT_RED;
    drawCentered(buffer, 120, pnlColor, 4);
    drawCentered("USDT", 160, TFT_WHITE, 2);
}

void Display::showAlert(const char* message) {
    tft.fillScreen(TFT_RED);
    drawCentered(message, 120, TFT_WHITE, 4);
}

void Display::formatPNL(int cents, char* buffer, size_t bufSize) {
    if (cents >= 0) {
        int dollars = cents / 100;
        int remainder = cents % 100;
        snprintf(buffer, bufSize, "$%d.%02d", dollars, remainder);
    } else {
        // Handle negative values correctly
        int absCents = -cents;
        int dollars = absCents / 100;
        int remainder = absCents % 100;
        snprintf(buffer, bufSize, "-$%d.%02d", dollars, remainder);
    }
}

void Display::formatPercent(int value, char* buffer, size_t bufSize) {
    int whole = value / 100;
    int frac = abs(value % 100);
    snprintf(buffer, bufSize, "%d.%02d%%", whole, frac);
}
