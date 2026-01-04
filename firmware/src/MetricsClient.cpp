#include "MetricsClient.h"
#include "Config.h"
#include <ArduinoJson.h>

MetricsClient::MetricsClient(const char* serverUrl) : failureCount(0) {
    strncpy(baseUrl, serverUrl, sizeof(baseUrl) - 1);
    baseUrl[sizeof(baseUrl) - 1] = '\0';
}

void MetricsClient::setServerUrl(const char* serverUrl) {
    strncpy(baseUrl, serverUrl, sizeof(baseUrl) - 1);
    baseUrl[sizeof(baseUrl) - 1] = '\0';
}

bool MetricsClient::fetchMetrics(MetricsData &data) {
    // Check if server URL is set
    if (baseUrl[0] == '\0') {
        return false;
    }
    
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
    
    // Validate response size against buffer capacity
    int contentLength = http.getSize();
    if (contentLength < 0 || contentLength >= METRICS_JSON_SIZE) {
        Serial.print(F("HTTP response too large or size unknown: "));
        Serial.println(contentLength);
        http.end();
        failureCount++;
        return false;
    }
    
    // Use getStream() to avoid String allocation
    WiFiClient* stream = http.getStreamPtr();
    char buffer[METRICS_JSON_SIZE];
    int len = stream->readBytes(buffer, contentLength);
    buffer[len] = '\0';
    
    http.end();
    
    bool success = parseMetrics(buffer, data);
    
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
