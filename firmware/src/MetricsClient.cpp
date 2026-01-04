#include "MetricsClient.h"
#include "Config.h"
#include <ArduinoJson.h>

MetricsClient::MetricsClient(const char* serverUrl) : failureCount(0) {
    strncpy(baseUrl, serverUrl, sizeof(baseUrl) - 1);
    baseUrl[sizeof(baseUrl) - 1] = '\0';
}

bool MetricsClient::fetchMetrics(MetricsData &data) {
    char url[160];
    snprintf(url, sizeof(url), "%s/api/v1/metrics", baseUrl);
    
    http.begin(wifiClient, url);
    http.setTimeout(HTTP_TIMEOUT_MS);
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.print(F("HTTP error: "));
        Serial.println(httpCode);
        http.end();
        failureCount++;
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    bool success = parseMetrics(payload.c_str(), data);
    
    if (success) {
        failureCount = 0;
    } else {
        failureCount++;
    }
    
    return success;
}

bool MetricsClient::parseMetrics(const char* json, MetricsData &data) {
    StaticJsonDocument<METRICS_JSON_SIZE> doc;
    
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.print(F("JSON parse error: "));
        Serial.println(error.c_str());
        return false;
    }
    
    // Parse all fields
    data.status = doc["s"] | 0;
    data.latency = doc["l"] | 0;
    data.activeTriangles = doc["a"] | 0;
    data.bestArb = doc["b"] | 0;
    data.pnl = doc["p"] | 0;
    data.errors = doc["e"] | 0;
    data.timestamp = doc["ts"] | 0;
    
    return true;
}
