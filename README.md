# ARB Desk Dashboard

A complete MVP desk dashboard system for monitoring an arbitrage trading bot, built for ESP8266 with a 240x240 TFT display.

## 📋 Overview

This project provides a reliable, 24/7 physical desk dashboard that displays real-time metrics from an arbitrage trading bot. The system consists of:

- **Firmware**: ESP8266-based embedded application with TFT display
- **Test Server**: Go HTTP server that simulates the bot backend
- **Protocol**: Well-defined JSON API contract

## ✨ Features

- **Real-time Metrics Display**
  - Bot status (OK/DOWN)
  - API latency monitoring
  - Active arbitrage opportunities
  - Best arbitrage percentage
  - Daily profit/loss (PNL)
  - Error tracking

- **Robust WiFi Management**
  - Auto-connect on boot
  - Captive portal for easy setup
  - Automatic reconnection
  - Clear status indicators

- **Alert System**
  - Visual alerts for bot failures
  - Network connectivity monitoring
  - Configurable failure thresholds

- **Memory-Safe Design**
  - Static JSON allocation
  - Minimal heap fragmentation
  - Suitable for 24/7 operation
  - Watchdog protection

## 🏗️ Repository Structure

```
arb-desk-dashboard/
├── firmware/           # ESP8266 firmware (Arduino/PlatformIO)
│   ├── src/           # Source code
│   ├── data/          # Filesystem data (config.json)
│   ├── platformio.ini # PlatformIO configuration
│   └── README.md      # Firmware documentation
├── testserver/        # Go test server
│   └── main.go        # Test server implementation
├── protocol/          # API specification
│   └── metrics.md     # Metrics endpoint documentation
└── README.md          # This file
```

## 🚀 Quick Start

### Prerequisites

- ESP8266-12F or compatible board (NodeMCU, Wemos D1 Mini, etc.)
- ST7789 240x240 TFT LCD display
- PlatformIO or Arduino IDE
- Go 1.16+ (for test server)

### 1. Hardware Setup

Connect the TFT display to ESP8266:

| TFT Pin | ESP8266 GPIO | NodeMCU Pin |
|---------|--------------|-------------|
| MOSI    | GPIO 13      | D7          |
| SCLK    | GPIO 14      | D5          |
| CS      | GPIO 15      | D8          |
| DC      | GPIO 5       | D1          |
| RST     | GPIO 4       | D2          |
| VCC     | 3.3V         | 3V3         |
| GND     | GND          | GND         |

> **Note:** Pin assignments can be customized in `firmware/platformio.ini`

### 2. Build & Flash Firmware

```bash
cd firmware
pio run --target upload
pio device monitor
```

See [firmware/README.md](firmware/README.md) for detailed instructions.

### 3. Initial Configuration

**Option A: Captive Portal (Easiest)**

1. Power on the device
2. Connect to WiFi network: `ARB-DASH-SETUP`
3. Browser should auto-open configuration page
4. Enter your WiFi credentials and server URL
5. Save and device will reboot

**Option B: Pre-configure Filesystem**

1. Edit `firmware/data/config.json`:
   ```json
   {
     "wifi": {
       "ssid": "YourWiFi",
       "pass": "YourPassword"
     },
     "server": {
       "url": "http://192.168.1.10:8080"
     },
     "refresh_ms": 3000
   }
   ```

2. Upload filesystem:
   ```bash
   pio run --target uploadfs
   ```

### 4. Run Test Server

Start the Go test server to simulate the bot backend:

```bash
cd testserver
go run main.go --port 8080 --mode ok --latency-ms 35
```

The test server provides three modes:
- `ok`: Normal operation with random metrics
- `down`: Simulates bot failure (status=0)
- `flap`: Randomly alternates between ok and down

Visit http://localhost:8080 in your browser to see the API endpoint.

### 5. Verify Operation

The ESP8266 display should show:
1. Boot screen
2. "Connecting to WiFi" with your SSID
3. "WiFi Connected" with IP address
4. Rotating dashboard screens:
   - STATUS (bot state, WiFi, latency)
   - ARBITRAGE (active opportunities, best %)
   - PNL (today's profit/loss)

## 📡 API Protocol

The ESP8266 polls the metrics endpoint via HTTP GET:

**Endpoint:** `GET /api/v1/metrics`

**Response:**
```json
{
  "s": 1,      // status: 1=ok, 0=down
  "l": 42,     // latency in ms
  "a": 3,      // active triangles
  "b": 18,     // best arb % × 100 (0.18%)
  "p": 482,    // PNL in cents ($4.82)
  "e": 0,      // errors in last 5 min
  "ts": 1735992000  // unix timestamp
}
```

See [protocol/metrics.md](protocol/metrics.md) for full specification.

## ⚙️ Configuration

### WiFi Settings

- **SSID**: 1-32 characters
- **Password**: 0-64 characters (can be empty)
- **Timeout**: 15 seconds (configurable)

### Server Settings

- **URL**: Must start with `http://` (no HTTPS/TLS)
- **Refresh interval**: 1000-15000ms (default: 3000ms)
- **Request timeout**: 2 seconds

### Alert Thresholds

- **Consecutive failures**: 3 failed HTTP requests
- **Bot down**: Server returns `s=0`

## 🔧 Customization

### Build-time Configuration

Edit `firmware/platformio.ini`:

```ini
build_flags = 
    -DDEFAULT_SERVER_URL=\"http://192.168.1.100:8080\"
    -DDEFAULT_AP_SSID=\"MY-CUSTOM-AP\"
    -DWIFI_CONNECT_TIMEOUT_MS=20000
```

### Display Pin Mapping

Adjust TFT pin assignments in `platformio.ini`:

```ini
-DTFT_MOSI=13
-DTFT_SCLK=14
-DTFT_CS=15
-DTFT_DC=5
-DTFT_RST=4
```

### Screen Rotation Timing

Modify constants in `firmware/src/Config.h`:

```cpp
#define SCREEN_ROTATION_MS 5000  // Time per screen
#define WIFI_STATUS_DISPLAY_MS 3000  // WiFi connected screen duration
```

## 🐛 Troubleshooting

### Display Issues
- Verify wiring matches pin configuration
- Check SPI frequency (reduce if unstable)
- Test with TFT_eSPI examples first

### WiFi Problems
- Ensure 2.4GHz network (ESP8266 doesn't support 5GHz)
- Check signal strength
- Verify credentials are correct
- Try increasing connection timeout

### Server Connection
- Confirm server URL format: `http://IP:PORT` (not https://)
- Test server is reachable from same network
- Check firewall rules
- Verify testserver is running: `curl http://SERVER_IP:8080/api/v1/metrics`

### Memory Issues
- Monitor free heap via serial: `ESP.getFreeHeap()`
- Reduce refresh rate if experiencing crashes
- Ensure all `yield()` calls are in place

## 📊 Test Server Usage

The included Go test server is useful for development and testing:

```bash
# Normal operation
go run main.go --port 8080 --mode ok

# Simulate bot down
go run main.go --port 8080 --mode down

# Simulate unstable bot (flapping)
go run main.go --port 8080 --mode flap

# Custom latency
go run main.go --port 8080 --latency-ms 100
```

Build standalone binary:
```bash
go build -o testserver main.go
./testserver --port 8080
```

## 🏭 Production Deployment

### Hardware Considerations

- Use quality power supply (5V, 500mA+)
- Consider case/enclosure for protection
- Ensure adequate ventilation
- Stable mounting for display viewing

### Backend Integration

Replace the test server with your actual bot backend:

1. Implement `GET /api/v1/metrics` endpoint
2. Return JSON in the specified format
3. Keep response < 256 bytes
4. Use HTTP (not HTTPS) for ESP8266
5. Respond within 2 seconds

### Monitoring

- Check serial output for errors
- Monitor WiFi signal strength
- Track consecutive failure counts
- Log API response times

## 🧪 Development

### Building Test Server

```bash
cd testserver
go build -o testserver main.go
```

### Cross-compiling

```bash
# For Linux
GOOS=linux GOARCH=amd64 go build -o testserver-linux main.go

# For Windows
GOOS=windows GOARCH=amd64 go build -o testserver.exe main.go

# For macOS
GOOS=darwin GOARCH=amd64 go build -o testserver-mac main.go
```

### Firmware Development

```bash
# Build without uploading
pio run

# Clean build
pio run --target clean

# Monitor serial output
pio device monitor --baud 115200
```

## 📝 Technical Specifications

### ESP8266 Constraints

- **RAM**: ~36KB available
- **Flash**: 4MB typical
- **WiFi**: 2.4GHz only, 802.11 b/g/n
- **Network**: HTTP only (no TLS)

### Performance

- **Screen update**: Every 5 seconds (normal mode)
- **Metrics polling**: Configurable (1-15 seconds)
- **WiFi reconnect**: Automatic
- **Uptime**: Designed for 24/7 operation

### Dependencies

**Firmware:**
- TFT_eSPI 2.5.43+
- ArduinoJson 6.21.3+
- ESP8266 Arduino Core

**Test Server:**
- Go 1.16+ standard library only

## 🔒 Security Notes

- System uses HTTP (no encryption)
- Designed for trusted LAN use only
- WiFi password stored in plaintext on device
- No authentication on API endpoints
- **Not suitable for public/internet deployment**

## 🤝 Contributing

This is an MVP implementation. Potential improvements:

- [ ] HTTPS/TLS support (requires ESP32)
- [ ] Authentication tokens
- [ ] Historical data graphing
- [ ] Touch screen controls
- [ ] Multiple bot support
- [ ] OTA firmware updates

## 📄 License

See LICENSE file for details.

## 🙏 Acknowledgments

- TFT_eSPI library by Bodmer
- ArduinoJson by Benoît Blanchon
- ESP8266 Arduino Core team

## 📧 Support

For issues and questions:
1. Check [firmware/README.md](firmware/README.md) for detailed documentation
2. Review [protocol/metrics.md](protocol/metrics.md) for API details
3. Enable serial debugging (115200 baud) for diagnostic output

---

**Built for reliability. Designed for 24/7 operation. Ready for arbitrage.**