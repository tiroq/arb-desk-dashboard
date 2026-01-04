# ARB Test Server

A simple HTTP server that simulates an arbitrage bot backend for testing the ESP8266 desk dashboard.

## Features

- Serves metrics via `/api/v1/metrics` endpoint
- Multiple operational modes (ok, down, flap)
- Configurable latency simulation
- Web interface showing current configuration and sample output

## Building

```bash
go build -o testserver main.go
```

Or just run directly:
```bash
go run main.go
```

## Usage

### Basic Usage

Start server on default port (8080) in normal operation mode:
```bash
./testserver
```

### Command-Line Flags

| Flag | Default | Description |
|------|---------|-------------|
| `--port` | 8080 | Server port to listen on |
| `--mode` | ok | Operational mode: `ok`, `down`, or `flap` |
| `--latency-ms` | 35 | Base latency in milliseconds |

### Examples

**Normal operation:**
```bash
./testserver --port 8080 --mode ok --latency-ms 35
```

**Simulate bot down:**
```bash
./testserver --port 8080 --mode down
```

**Simulate flapping (unstable) bot:**
```bash
./testserver --port 8080 --mode flap
```

**Custom port and latency:**
```bash
./testserver --port 9000 --latency-ms 100
```

## Operational Modes

### `ok` Mode (Default)
Returns normal metrics with random variations:
- Status: 1 (running)
- Latency: Base latency ± 10ms
- Active triangles: 1-15
- Best arbitrage: 0.05% - 0.85%
- PnL: $0 - $50
- Errors: 0-2

Example response:
```json
{
  "s": 1,
  "l": 42,
  "a": 5,
  "b": 25,
  "p": 1234,
  "e": 0,
  "ts": 1735992123
}
```

### `down` Mode
Simulates bot failure:
- Status: 0 (down)
- All metrics zeroed except errors
- Errors: 5-24

Example response:
```json
{
  "s": 0,
  "l": 0,
  "a": 0,
  "b": 0,
  "p": 0,
  "e": 15,
  "ts": 1735992456
}
```

### `flap` Mode
Randomly alternates between ok and down states on each request. Useful for testing alert behavior and edge cases.

## Endpoints

### `GET /`
Web interface showing:
- Current server configuration
- Link to metrics endpoint
- Sample JSON response

### `GET /api/v1/metrics`
Returns current bot metrics in JSON format.

**Response Schema:**
```json
{
  "s": 1,      // status: 1=ok, 0=down
  "l": 42,     // latency in ms
  "a": 3,      // active triangular arbitrage opportunities
  "b": 18,     // best arbitrage percentage × 100 (0.18%)
  "p": 482,    // profit/loss today in cents ($4.82)
  "e": 0,      // error count (last 5 minutes)
  "ts": 1735992000  // unix timestamp
}
```

See [../protocol/metrics.md](../protocol/metrics.md) for full API specification.

## Testing with cURL

```bash
# Get metrics
curl http://localhost:8080/api/v1/metrics

# Pretty print JSON
curl -s http://localhost:8080/api/v1/metrics | jq .

# Watch metrics update in real-time
watch -n 1 'curl -s http://localhost:8080/api/v1/metrics | jq .'
```

## Testing with ESP8266

1. Start the test server on your development machine:
   ```bash
   ./testserver --port 8080 --mode ok
   ```

2. Find your machine's IP address:
   ```bash
   # Linux/Mac
   ip addr show
   # or
   ifconfig
   
   # Windows
   ipconfig
   ```

3. Configure ESP8266 to use this IP and port:
   ```
   http://YOUR_IP_ADDRESS:8080
   ```

4. The ESP8266 will poll `/api/v1/metrics` at the configured interval

## Production Usage

This server is for **testing only**. For production:

1. Implement the same API endpoint in your actual bot backend
2. Ensure responses are < 256 bytes
3. Keep response time under 2 seconds
4. Use HTTP (not HTTPS) for ESP8266 compatibility
5. Deploy on same network as ESP8266

## Cross-Compiling

Build for different platforms:

```bash
# Linux (64-bit)
GOOS=linux GOARCH=amd64 go build -o testserver-linux

# Windows (64-bit)
GOOS=windows GOARCH=amd64 go build -o testserver.exe

# macOS (64-bit)
GOOS=darwin GOARCH=amd64 go build -o testserver-mac

# Raspberry Pi (ARM)
GOOS=linux GOARCH=arm GOARM=7 go build -o testserver-rpi
```

## Development

The server uses only Go standard library (`net/http`, `encoding/json`, `flag`), so no external dependencies are required.

Modify `main.go` to:
- Add new operational modes
- Adjust metric ranges
- Customize response patterns
- Add logging or monitoring

## License

See main repository LICENSE.
