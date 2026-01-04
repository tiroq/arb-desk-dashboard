#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

// Default server URL from build flags
#ifndef DEFAULT_SERVER_URL
#define DEFAULT_SERVER_URL "http://192.168.1.100:8080"
#endif

// Configuration structure
struct Config {
  char wifi_ssid[32];
  char wifi_pass[64];
  char server_url[128];
  unsigned long refresh_ms;
};

// Global objects
TFT_eSPI tft = TFT_eSPI();
Config config;
ESP8266WebServer server(80);
DNSServer dnsServer;
WiFiClient wifiClient;

// State variables
bool configMode = false;
bool botDown = false;
bool fetchFailed = false;
unsigned long lastFetch = 0;

// Metrics structure
struct Metrics {
  float s;  // spread
  float l;  // liquidity
  float a;  // asset_a
  float b;  // asset_b
  float p;  // profit
  float e;  // efficiency
  unsigned long ts; // timestamp
};

Metrics metrics;

// Function declarations
void loadConfig();
void saveConfig();
void startConfigPortal();
void connectWiFi();
void fetchMetrics();
void displayMetrics();
void displayAlert(const char* message);
void handleRoot();
void handleSave();

void setup() {
  Serial.begin(115200);
  Serial.println("\nARB Desk Dashboard Starting...");
  
  // Initialize display
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 100);
  tft.println("ARB DASH");
  tft.setCursor(10, 130);
  tft.println("Starting...");
  
  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    tft.fillScreen(TFT_RED);
    tft.setCursor(10, 100);
    tft.println("FS ERROR");
    while (1) delay(1000);
  }
  
  // Load configuration
  loadConfig();
  
  // Try to connect to WiFi
  connectWiFi();
}

void loop() {
  if (configMode) {
    dnsServer.processNextRequest();
    server.handleClient();
  } else {
    // Check if it's time to fetch metrics
    if (millis() - lastFetch >= config.refresh_ms) {
      fetchMetrics();
      lastFetch = millis();
    }
    
    // Update display
    if (fetchFailed || botDown) {
      displayAlert(fetchFailed ? "FETCH FAIL" : "BOT DOWN");
    } else {
      displayMetrics();
    }
    
    delay(100);
  }
}

void loadConfig() {
  if (LittleFS.exists("/config.json")) {
    File file = LittleFS.open("/config.json", "r");
    if (file) {
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, file);
      file.close();
      
      if (!error) {
        strlcpy(config.wifi_ssid, doc["ssid"] | "", sizeof(config.wifi_ssid));
        strlcpy(config.wifi_pass, doc["pass"] | "", sizeof(config.wifi_pass));
        strlcpy(config.server_url, doc["server_url"] | DEFAULT_SERVER_URL, sizeof(config.server_url));
        config.refresh_ms = doc["refresh_ms"] | 5000;
        Serial.println("Config loaded");
        return;
      }
    }
  }
  
  // Use defaults if no config found
  Serial.println("Using default config");
  config.wifi_ssid[0] = '\0';
  config.wifi_pass[0] = '\0';
  strlcpy(config.server_url, DEFAULT_SERVER_URL, sizeof(config.server_url));
  config.refresh_ms = 5000;
}

void saveConfig() {
  File file = LittleFS.open("/config.json", "w");
  if (!file) {
    Serial.println("Failed to create config file");
    return;
  }
  
  StaticJsonDocument<256> doc;
  doc["ssid"] = config.wifi_ssid;
  doc["pass"] = config.wifi_pass;
  doc["server_url"] = config.server_url;
  doc["refresh_ms"] = config.refresh_ms;
  
  serializeJson(doc, file);
  file.close();
  Serial.println("Config saved");
}

void connectWiFi() {
  if (strlen(config.wifi_ssid) == 0) {
    Serial.println("No WiFi credentials, starting config portal");
    startConfigPortal();
    return;
  }
  
  Serial.print("Connecting to WiFi: ");
  Serial.println(config.wifi_ssid);
  
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 100);
  tft.println("WiFi...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifi_ssid, config.wifi_pass);
  
  // Wait up to 15 seconds for connection
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed, starting config portal");
    startConfigPortal();
  } else {
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 100);
    tft.println("Connected");
    tft.setCursor(10, 130);
    tft.println(WiFi.localIP().toString());
    delay(2000);
  }
}

void startConfigPortal() {
  configMode = true;
  
  tft.fillScreen(TFT_BLUE);
  tft.setTextSize(2);
  tft.setCursor(10, 40);
  tft.println("CONFIG MODE");
  tft.setCursor(10, 80);
  tft.setTextSize(1);
  tft.println("Connect to:");
  tft.setCursor(10, 100);
  tft.setTextSize(2);
  tft.println("ARB-DASH");
  tft.println("  -SETUP");
  tft.setCursor(10, 150);
  tft.setTextSize(1);
  tft.println("Open browser:");
  tft.setCursor(10, 170);
  tft.setTextSize(2);
  tft.println("192.168.4.1");
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ARB-DASH-SETUP");
  
  Serial.println("AP started: ARB-DASH-SETUP");
  Serial.println("IP: 192.168.4.1");
  
  // Start DNS server for captive portal
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleRoot);
  server.begin();
  
  Serial.println("Config portal started");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ARB Dash Setup</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>body{font-family:Arial;padding:20px;background:#f0f0f0;}"
    "form{background:white;padding:20px;border-radius:5px;max-width:400px;}"
    "input{width:100%;padding:8px;margin:5px 0 15px;box-sizing:border-box;}"
    "button{background:#4CAF50;color:white;padding:10px 20px;border:none;cursor:pointer;width:100%;}"
    "button:hover{background:#45a049;}</style></head><body>"
    "<form action='/save' method='POST'>"
    "<h2>ARB Dash Setup</h2>"
    "<label>WiFi SSID:</label><input name='ssid' value='" + String(config.wifi_ssid) + "'>"
    "<label>WiFi Password:</label><input type='password' name='pass'>"
    "<label>Server URL:</label><input name='url' value='" + String(config.server_url) + "'>"
    "<label>Refresh (ms):</label><input name='refresh' type='number' value='" + String(config.refresh_ms) + "'>"
    "<button type='submit'>Save & Reboot</button>"
    "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("ssid")) {
    strlcpy(config.wifi_ssid, server.arg("ssid").c_str(), sizeof(config.wifi_ssid));
  }
  if (server.hasArg("pass")) {
    strlcpy(config.wifi_pass, server.arg("pass").c_str(), sizeof(config.wifi_pass));
  }
  if (server.hasArg("url")) {
    strlcpy(config.server_url, server.arg("url").c_str(), sizeof(config.server_url));
  }
  if (server.hasArg("refresh")) {
    config.refresh_ms = server.arg("refresh").toInt();
    if (config.refresh_ms < 1000) config.refresh_ms = 1000;
  }
  
  saveConfig();
  
  String html = "<!DOCTYPE html><html><head><title>Saved</title>"
    "<meta http-equiv='refresh' content='3;url=/'></head><body>"
    "<h2>Configuration saved! Rebooting...</h2></body></html>";
  server.send(200, "text/html", html);
  
  delay(2000);
  ESP.restart();
}

void fetchMetrics() {
  HTTPClient http;
  
  String url = String(config.server_url) + "/api/v1/metrics";
  Serial.print("Fetching: ");
  Serial.println(url);
  
  http.begin(wifiClient, url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Response: " + payload);
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      metrics.s = doc["s"] | 0.0;
      metrics.l = doc["l"] | 0.0;
      metrics.a = doc["a"] | 0.0;
      metrics.b = doc["b"] | 0.0;
      metrics.p = doc["p"] | 0.0;
      metrics.e = doc["e"] | 0.0;
      metrics.ts = doc["ts"] | 0;
      
      fetchFailed = false;
      
      // Check if bot is down (timestamp too old or all zeros)
      unsigned long currentTime = millis() / 1000;
      if (metrics.ts == 0 || (currentTime > metrics.ts && currentTime - metrics.ts > 300)) {
        botDown = true;
      } else {
        botDown = false;
      }
    } else {
      Serial.println("JSON parse error");
      fetchFailed = true;
    }
  } else {
    Serial.print("HTTP error: ");
    Serial.println(httpCode);
    fetchFailed = true;
  }
  
  http.end();
}

void displayMetrics() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  
  // Title
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(60, 5);
  tft.println("ARB DESK METRICS");
  
  // Draw separator
  tft.drawLine(0, 20, 240, 20, TFT_DARKGREY);
  
  int y = 30;
  int lineHeight = 30;
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  
  // Spread
  tft.setCursor(10, y);
  tft.print("Spread:");
  tft.setCursor(140, y);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print(metrics.s, 4);
  y += lineHeight;
  
  // Liquidity
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, y);
  tft.print("Liquid:");
  tft.setCursor(140, y);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.print(metrics.l, 2);
  y += lineHeight;
  
  // Asset A
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, y);
  tft.print("Asset A:");
  tft.setCursor(140, y);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.print(metrics.a, 2);
  y += lineHeight;
  
  // Asset B
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, y);
  tft.print("Asset B:");
  tft.setCursor(140, y);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.print(metrics.b, 2);
  y += lineHeight;
  
  // Profit
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, y);
  tft.print("Profit:");
  tft.setCursor(140, y);
  tft.setTextColor(metrics.p >= 0 ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.print(metrics.p, 2);
  y += lineHeight;
  
  // Efficiency
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, y);
  tft.print("Effic:");
  tft.setCursor(140, y);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.print(metrics.e, 2);
  tft.print("%");
  y += lineHeight;
  
  // Timestamp
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 220);
  tft.print("TS: ");
  tft.print(metrics.ts);
}

void displayAlert(const char* message) {
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.setTextSize(3);
  tft.setCursor(20, 80);
  tft.println("ALERT!");
  tft.setTextSize(2);
  tft.setCursor(20, 120);
  tft.println(message);
  
  // Blink effect
  static unsigned long lastBlink = 0;
  static bool blinkState = false;
  if (millis() - lastBlink > 500) {
    blinkState = !blinkState;
    lastBlink = millis();
    if (blinkState) {
      tft.fillRect(0, 0, 240, 20, TFT_YELLOW);
    } else {
      tft.fillRect(0, 0, 240, 20, TFT_RED);
    }
  }
}
