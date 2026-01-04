#include "Config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

bool loadConfig(AppConfig &config) {
    // Set defaults first
    strncpy(config.wifi.ssid, "", sizeof(config.wifi.ssid));
    strncpy(config.wifi.password, "", sizeof(config.wifi.password));
    strncpy(config.server.url, DEFAULT_SERVER_URL, sizeof(config.server.url));
    config.refresh_ms = 3000;

    if (!LittleFS.begin()) {
        Serial.println(F("LittleFS mount failed"));
        return false;
    }

    if (!LittleFS.exists(CONFIG_FILE_PATH)) {
        Serial.println(F("Config file not found, using defaults"));
        return false;
    }

    File file = LittleFS.open(CONFIG_FILE_PATH, "r");
    if (!file) {
        Serial.println(F("Failed to open config file"));
        return false;
    }

    StaticJsonDocument<CONFIG_JSON_SIZE> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print(F("Config parse error: "));
        Serial.println(error.c_str());
        return false;
    }

    // Parse WiFi config
    if (doc.containsKey("wifi")) {
        JsonObject wifi = doc["wifi"];
        const char* ssid = wifi["ssid"] | "";
        const char* pass = wifi["pass"] | "";
        strncpy(config.wifi.ssid, ssid, sizeof(config.wifi.ssid) - 1);
        strncpy(config.wifi.password, pass, sizeof(config.wifi.password) - 1);
    }

    // Parse server config
    if (doc.containsKey("server")) {
        JsonObject server = doc["server"];
        const char* url = server["url"] | DEFAULT_SERVER_URL;
        strncpy(config.server.url, url, sizeof(config.server.url) - 1);
    }

    // Parse refresh interval
    config.refresh_ms = doc["refresh_ms"] | 3000;

    return validateConfig(config);
}

bool saveConfig(const AppConfig &config) {
    if (!validateConfig(config)) {
        Serial.println(F("Invalid config, cannot save"));
        return false;
    }

    StaticJsonDocument<CONFIG_JSON_SIZE> doc;

    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = config.wifi.ssid;
    wifi["pass"] = config.wifi.password;

    JsonObject server = doc.createNestedObject("server");
    server["url"] = config.server.url;

    doc["refresh_ms"] = config.refresh_ms;

    File file = LittleFS.open(CONFIG_FILE_PATH, "w");
    if (!file) {
        Serial.println(F("Failed to open config file for writing"));
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println(F("Failed to write config"));
        file.close();
        return false;
    }

    file.close();
    Serial.println(F("Config saved successfully"));
    return true;
}

bool validateConfig(const AppConfig &config) {
    // Validate SSID length
    size_t ssidLen = strlen(config.wifi.ssid);
    if (ssidLen < MIN_SSID_LEN || ssidLen > MAX_SSID_LEN) {
        Serial.println(F("Invalid SSID length"));
        return false;
    }

    // Validate password length
    size_t passLen = strlen(config.wifi.password);
    if (passLen > MAX_PASS_LEN) {
        Serial.println(F("Invalid password length"));
        return false;
    }

    // Validate URL starts with http://
    if (strncmp(config.server.url, "http://", 7) != 0) {
        Serial.println(F("URL must start with http://"));
        return false;
    }

    // Validate refresh interval
    if (config.refresh_ms < MIN_REFRESH_MS || config.refresh_ms > MAX_REFRESH_MS) {
        Serial.println(F("Invalid refresh interval"));
        return false;
    }

    return true;
}
