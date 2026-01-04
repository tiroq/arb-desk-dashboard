# Metrics Protocol Specification

## Overview
This document defines the HTTP API contract between the ESP8266 desk dashboard and the arbitrage trading bot backend.

## API Endpoint

### GET /api/v1/metrics

Returns current bot metrics in JSON format.

**Protocol:** HTTP (no TLS)  
**Method:** GET  
**Path:** `/api/v1/metrics`  
**Response Type:** `application/json`

## Response Schema

```json
{
  "s": 1,
  "l": 42,
  "a": 3,
  "b": 18,
  "p": 482,
  "e": 0,
  "ts": 1735992000
}
```

### Field Definitions

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| `s` | int | Bot status: 1=running/ok, 0=down | 1 |
| `l` | int | Latency in milliseconds | 42 |
| `a` | int | Number of active triangular arbitrage opportunities | 3 |
| `b` | int | Best arbitrage percentage × 100 (i.e., 18 = 0.18%) | 18 |
| `p` | int | Profit and Loss for today in cents (482 = $4.82) | 482 |
| `e` | int | Error count in the last 5 minutes | 0 |
| `ts` | int | Unix timestamp (epoch seconds) of this snapshot | 1735992000 |

## Constraints

- **Maximum payload size:** < 256 bytes (enforced for ESP8266 memory constraints)
- **All numeric values:** Integer only (no floats)
- **Network:** HTTP only, no HTTPS/TLS required (LAN usage)
- **Timeout:** Client will timeout after ~2 seconds

## Client Behavior

The ESP8266 client will:
1. Poll this endpoint at a configurable interval (default 3000ms, range 1000-15000ms)
2. Parse the JSON response using StaticJsonDocument
3. Track consecutive failures
4. Enter alert mode after 3 consecutive failures OR if `s == 0`

## Error Handling

If the backend cannot provide valid metrics:
- Return HTTP 503 Service Unavailable
- Or return `"s": 0` to indicate bot is down

## Example Responses

### Normal Operation
```json
{
  "s": 1,
  "l": 35,
  "a": 5,
  "b": 22,
  "p": 1250,
  "e": 0,
  "ts": 1735992123
}
```

### Bot Down
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

### High Performance
```json
{
  "s": 1,
  "l": 18,
  "a": 12,
  "b": 45,
  "p": 3890,
  "e": 0,
  "ts": 1735992789
}
```
