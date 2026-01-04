# arb-desk-dashboard

ESP8266-based arbitrage trading desk dashboard with ST7789 240x240 TFT display.

## Overview

This project provides a hardware dashboard for monitoring arbitrage trading bot metrics in real-time. It consists of:

1. **ESP8266 Firmware** - Displays real-time metrics on a TFT screen
2. **Go Test Server** - Provides sample API endpoint for testing

## Hardware Requirements

- ESP8266-12F module
- ST7789 240x240 TFT display
- Power supply (5V USB or 3.3V regulated)

## Pin Configuration

Default pin connections for ST7789 display:

| Display Pin | ESP8266 Pin |
|-------------|-------------|
| MOSI        | GPIO 13     |
| SCLK        | GPIO 14     |
| CS          | GPIO 15     |
| DC          | GPIO 4      |
| RST         | GPIO 5      |

## Features

### ESP8266 Firmware

- **WiFi Management**: Auto-connect with fallback to captive portal
- **Configuration Portal**: 
  - SSID: `ARB-DASH-SETUP`
  - IP: `192.168.4.1`
  - Configure WiFi credentials, server URL, and refresh interval
- **LittleFS Storage**: Persistent configuration storage
- **Real-time Metrics Display**:
  - Spread (s)
  - Liquidity (l)
  - Asset A balance (a)
  - Asset B balance (b)
  - Profit (p)
  - Efficiency (e)
  - Timestamp (ts)
- **Alert System**: Visual alerts for bot failures or connection issues
- **Compact JSON**: Uses StaticJsonDocument (<256 bytes)

### Configuration

The device can be configured in two ways:

1. **Build-time default**: Set `DEFAULT_SERVER_URL` in `platformio.ini`
2. **Runtime config**: Use the captive portal at `192.168.4.1`

Configuration is stored in `/config.json` on the LittleFS filesystem.

### API Endpoint

The firmware expects a JSON endpoint at: `GET /api/v1/metrics`

Response format:
```json
{
  "s": 0.0025,
  "l": 125000.50,
  "a": 50000.25,
  "b": 75000.75,
  "p": 342.18,
  "e": 94.5,
  "ts": 1767538293
}
```

All values are numbers only (no strings). Response must be < 256 bytes.

## Building and Flashing

### Prerequisites

- [PlatformIO](https://platformio.org/) installed
- USB-to-serial adapter (if not built into your ESP8266 board)

### Build Firmware

```bash
# Install PlatformIO if not already installed
pip install platformio

# Build the firmware
cd /path/to/arb-desk-dashboard
pio run

# Upload to ESP8266
pio run --target upload

# Monitor serial output
pio device monitor
```

### Configure Default Server URL

Edit `platformio.ini` and modify the build flag:

```ini
build_flags = 
    -DDEFAULT_SERVER_URL=\"http://YOUR_SERVER_IP:8080\"
```

## Go Test Server

A simple test server is provided for development and testing.

### Run Test Server

```bash
cd server
go run main.go
```

The server will start on `http://localhost:8080` with the metrics endpoint at `/api/v1/metrics`.

### Run Tests

```bash
cd server
go test -v
```

### Build Test Server

```bash
cd server
go build -o test-server
./test-server
```

## Usage

### First Boot

1. Flash the firmware to ESP8266
2. Power on the device
3. If no WiFi is configured, it will start in AP mode
4. Connect to WiFi network `ARB-DASH-SETUP`
5. Open browser to `http://192.168.4.1`
6. Enter WiFi credentials and server URL
7. Click "Save & Reboot"
8. Device will restart and connect to your WiFi

### Normal Operation

- Device connects to configured WiFi
- Fetches metrics from configured server every N milliseconds (default: 5000ms)
- Displays metrics on TFT screen with color-coded values
- Shows alert screen if:
  - Metrics fetch fails (network/server error)
  - Bot appears down (timestamp too old or zero)

### Reconfiguration

To reconfigure the device:

1. Power off the device
2. Edit `/config.json` on LittleFS, or
3. Erase the flash to reset to defaults and trigger AP mode

## Display Layout

```
┌─────────────────────────┐
│  ARB DESK METRICS       │ (Cyan header)
├─────────────────────────┤
│ Spread:       0.0025    │ (Green)
│ Liquid:     125000.50   │ (Yellow)
│ Asset A:     50000.25   │ (Cyan)
│ Asset B:     75000.75   │ (Cyan)
│ Profit:        342.18   │ (Green/Red based on value)
│ Effic:          94.5%   │ (Magenta)
│                         │
│ TS: 1767538293          │ (Gray)
└─────────────────────────┘
```

## Alert Screen

When errors occur, the display shows a red alert screen with blinking header:

```
┌─────────────────────────┐
│   [BLINKING HEADER]     │
│                         │
│      ALERT!             │
│                         │
│    FETCH FAIL           │
│    or BOT DOWN          │
│                         │
└─────────────────────────┘
```

## Troubleshooting

### WiFi Won't Connect

- Verify SSID and password are correct
- Check WiFi signal strength
- Ensure 2.4GHz WiFi (ESP8266 doesn't support 5GHz)
- Wait full 15 seconds for connection attempt

### Display Issues

- Check pin connections
- Verify 3.3V power supply is stable
- Review TFT_eSPI configuration in `platformio.ini`

### Metrics Not Updating

- Verify server URL is accessible from ESP8266's network
- Check server is running and responding
- Monitor serial output for error messages
- Verify JSON response is < 256 bytes

### Can't Access Config Portal

- Ensure device is in AP mode (blue screen)
- Connect to `ARB-DASH-SETUP` network
- Try `http://192.168.4.1` in browser
- Clear browser cache if needed

## License

See LICENSE file for details.

## Development

### Project Structure

```
arb-desk-dashboard/
├── platformio.ini          # PlatformIO configuration
├── src/
│   └── main.cpp           # ESP8266 firmware
├── server/
│   ├── main.go            # Go test server
│   ├── main_test.go       # Go tests
│   └── go.mod             # Go module definition
└── README.md              # This file
```

### Adding Features

When modifying the firmware:

1. Keep JSON responses compact (<256 bytes)
2. Use StaticJsonDocument for parsing
3. Test WiFi failure scenarios
4. Verify display updates work correctly
5. Test configuration portal functionality

### API Server Requirements

Production servers must:

- Respond to `GET /api/v1/metrics`
- Return JSON with fields: s, l, a, b, p, e, ts
- Use numeric values only (no strings)
- Keep response < 256 bytes
- Support HTTP (no TLS required for LAN use)
- Update timestamp (ts) regularly to indicate bot is alive
