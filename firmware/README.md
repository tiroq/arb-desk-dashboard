# ARB Desk Dashboard - Firmware

ESP8266-based desk dashboard for monitoring an arbitrage trading bot.

## Hardware Requirements

- **MCU:** ESP8266-12F (or compatible like NodeMCU, Wemos D1 Mini)
- **Display:** ST7789 240x240 TFT LCD
- **Connections:**
  - TFT_MOSI → GPIO 13 (D7)
  - TFT_SCLK → GPIO 14 (D5)
  - TFT_CS → GPIO 15 (D8)
  - TFT_DC → GPIO 5 (D1)
  - TFT_RST → GPIO 4 (D2)

> Note: Pin assignments can be customized in `platformio.ini` build flags.

## Software Requirements

- [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE
- USB drivers for ESP8266 (CH340/CP2102 depending on your board)

## Building & Flashing

### Using PlatformIO (Recommended)

1. **Install PlatformIO:**
   ```bash
   pip install platformio
   ```

2. **Build the firmware:**
   ```bash
   cd firmware
   pio run
   ```

3. **Flash to device:**
   ```bash
   pio run --target upload
   ```

4. **Monitor serial output:**
   ```bash
   pio device monitor
   ```

### Using PlatformIO IDE (VSCode)

1. Install PlatformIO IDE extension in VSCode
2. Open the `firmware` folder
3. Click "Build" (checkmark icon)
4. Click "Upload" (arrow icon)
5. Click "Serial Monitor" (plug icon)

## First-Time Setup

### Option 1: Pre-configure via filesystem

1. **Prepare config file:**
   - Edit `data/config.json` with your WiFi credentials and server URL
   
2. **Upload filesystem:**
   ```bash
   pio run --target uploadfs
   ```

3. **Flash firmware** (see above)

### Option 2: Captive Portal Setup

1. Flash firmware without pre-configured WiFi
2. Device will start in AP mode
3. Connect to WiFi network: **ARB-DASH-SETUP**
4. Browser should auto-open to 192.168.4.1
5. If not, navigate to: http://192.168.4.1
6. Fill in the configuration form:
   - WiFi SSID
   - WiFi Password
   - Server URL (e.g., `http://192.168.1.10:8080`)
   - Refresh interval (1000-15000ms)
7. Click "Save & Reboot"
8. Device will restart and connect to your WiFi

## Configuration

### Config File Format

Location: `/config.json` (in LittleFS filesystem)

```json
{
  "wifi": {
    "ssid": "MyWiFi",
    "pass": "MyPassword"
  },
  "server": {
    "url": "http://192.168.1.10:8080"
  },
  "refresh_ms": 3000
}
```

### Validation Rules

- **SSID:** 1-32 characters
- **Password:** 0-64 characters (can be empty for open networks)
- **URL:** Must start with `http://`
- **Refresh interval:** 1000-15000 milliseconds

### Build-time Defaults

These can be customized in `platformio.ini`:

```ini
-DDEFAULT_SERVER_URL=\"http://192.168.1.10:8080\"
-DDEFAULT_AP_SSID=\"ARB-DASH-SETUP\"
-DWIFI_CONNECT_TIMEOUT_MS=15000
```

## Operation

### Normal Mode

The display rotates through three screens every 5 seconds:

1. **STATUS Screen**
   - Bot status (OK/DOWN)
   - WiFi signal strength (dBm)
   - API latency (ms)

2. **ARBITRAGE Screen**
   - Active triangular opportunities
   - Best arbitrage percentage

3. **PNL Screen**
   - Today's profit/loss in USDT

### Alert Mode

The display shows a full-screen red alert when:
- HTTP fetch fails 3 times consecutively, OR
- Bot status is DOWN (s=0)

Messages:
- **"NO DATA"** - Cannot reach server
- **"BOT DOWN"** - Bot reported as down

### WiFi Reconnection

If WiFi disconnects during operation:
- Display shows "Connecting to WiFi"
- Automatic reconnection attempts
- Falls back to alert if unable to reconnect

## Troubleshooting

### Display not working
- Check wiring connections
- Verify TFT_eSPI pin configuration matches your hardware
- Try adjusting SPI frequency in `platformio.ini`

### WiFi won't connect
- Verify SSID and password are correct
- Check WiFi signal strength
- Ensure router supports 2.4GHz (ESP8266 doesn't support 5GHz)
- Try increasing `WIFI_CONNECT_TIMEOUT_MS`

### Cannot reach server
- Verify server URL is correct (must start with `http://`)
- Ensure server is on same network
- Check firewall settings
- Test with testserver (see main README)

### Watchdog resets / crashes
- Ensure `yield()` is called regularly (already implemented)
- Check memory usage (use Serial monitor)
- Reduce refresh rate if needed

### Filesystem errors
- Re-flash filesystem: `pio run --target uploadfs`
- Ensure LittleFS is properly mounted
- Check for corruption (may need to erase flash)

## Memory Considerations

ESP8266 has limited RAM (~36KB available). The firmware is designed with this in mind:

- Static JSON documents (no dynamic allocation)
- Minimal use of Arduino String class
- Fixed-size buffers for WiFi/config data
- Careful management of HTTP client lifecycle

## Serial Debug Output

Connect via serial monitor (115200 baud) to see:
- Boot sequence
- WiFi connection status
- Config loading
- HTTP request results
- Error messages

Example output:
```
=== ARB Desk Dashboard ===
WiFi connected
IP: 192.168.1.50
Setup complete, entering main loop
```

## LED Indicators (if available on your board)

Most ESP8266 boards have a built-in LED:
- Rapid blinking during WiFi connection
- Slow blink during normal operation
- Solid on/off depends on board variant

## Power Supply

- Use quality 5V power supply (minimum 500mA recommended)
- USB power is usually sufficient
- For production, consider regulated power supply

## License

See main repository README for license information.
