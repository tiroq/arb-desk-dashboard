#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Build-time defaults (can be overridden by build flags)
#ifndef DEFAULT_SERVER_URL
#define DEFAULT_SERVER_URL "http://192.168.1.10:8080"
#endif

#ifndef DEFAULT_AP_SSID
#define DEFAULT_AP_SSID "ARB-DASH-SETUP"
#endif

#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 15000
#endif

// Validation constraints
#define MIN_SSID_LEN 1
#define MAX_SSID_LEN 32
#define MIN_PASS_LEN 0
#define MAX_PASS_LEN 64
#define MIN_REFRESH_MS 1000
#define MAX_REFRESH_MS 15000

// File paths
#define CONFIG_FILE_PATH "/config.json"

// JSON buffer sizes
#define CONFIG_JSON_SIZE 256
#define METRICS_JSON_SIZE 256

// Network settings
#define HTTP_TIMEOUT_MS 2000
#define MAX_CONSECUTIVE_FAILURES 3

// UI settings
#define SCREEN_ROTATION_MS 5000
#define WIFI_STATUS_DISPLAY_MS 3000

// SoftAP settings
#define SOFTAP_IP_ADDR 192,168,4,1

struct WifiConfig {
    char ssid[MAX_SSID_LEN + 1];
    char password[MAX_PASS_LEN + 1];
};

struct ServerConfig {
    char url[128];
};

struct AppConfig {
    WifiConfig wifi;
    ServerConfig server;
    uint16_t refresh_ms;
};

// Config management functions
bool loadConfig(AppConfig &config);
bool saveConfig(const AppConfig &config);
bool validateConfig(const AppConfig &config);

#endif // CONFIG_H
