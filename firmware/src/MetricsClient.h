#ifndef METRICS_CLIENT_H
#define METRICS_CLIENT_H

#include "Display.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

class MetricsClient {
public:
    MetricsClient() : failureCount(0) { baseUrl[0] = '\0'; }
    MetricsClient(const char* serverUrl);
    
    void setServerUrl(const char* serverUrl);
    bool fetchMetrics(MetricsData &data);
    int getFailureCount() const { return failureCount; }
    void resetFailureCount() { failureCount = 0; }

private:
    char baseUrl[128];
    int failureCount;
    WiFiClient wifiClient;
    HTTPClient http;
    
    bool parseMetrics(const char* json, MetricsData &data);
};

#endif // METRICS_CLIENT_H
