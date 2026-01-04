package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"math/rand"
	"net/http"
	"time"
)

// MetricsResponse represents the API response structure
type MetricsResponse struct {
	Status          int   `json:"s"`  // 1=ok, 0=down
	Latency         int   `json:"l"`  // ms
	ActiveTriangles int   `json:"a"`  // count
	BestArb         int   `json:"b"`  // percentage × 100
	PNL             int   `json:"p"`  // cents
	Errors          int   `json:"e"`  // count
	Timestamp       int64 `json:"ts"` // epoch seconds
}

var (
	port      = flag.Int("port", 8080, "Server port")
	mode      = flag.String("mode", "ok", "Server mode: ok, down, or flap")
	latencyMs = flag.Int("latency-ms", 35, "Base latency in milliseconds")
	rng       = rand.New(rand.NewSource(time.Now().UnixNano()))
)

func main() {
	flag.Parse()

	http.HandleFunc("/", handleRoot)
	http.HandleFunc("/api/v1/metrics", handleMetrics)

	addr := fmt.Sprintf(":%d", *port)
	log.Printf("Starting ARB test server on %s", addr)
	log.Printf("Mode: %s, Base Latency: %dms", *mode, *latencyMs)
	log.Printf("Metrics endpoint: http://localhost%s/api/v1/metrics", addr)

	if err := http.ListenAndServe(addr, nil); err != nil {
		log.Fatal(err)
	}
}

func handleRoot(w http.ResponseWriter, r *http.Request) {
	html := `<!DOCTYPE html>
<html>
<head>
    <title>ARB Test Server</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #333; }
        .info { background: #e7f3ff; padding: 15px; border-radius: 4px; margin: 20px 0; }
        .metrics { background: #f0f0f0; padding: 15px; border-radius: 4px; font-family: monospace; }
        pre { margin: 0; }
        a { color: #007bff; text-decoration: none; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="container">
        <h1>📊 ARB Test Server</h1>
        <div class="info">
            <strong>Mode:</strong> ` + *mode + `<br>
            <strong>Base Latency:</strong> ` + fmt.Sprintf("%d", *latencyMs) + `ms<br>
            <strong>Port:</strong> ` + fmt.Sprintf("%d", *port) + `
        </div>
        <h2>API Endpoint</h2>
        <p><a href="/api/v1/metrics">/api/v1/metrics</a></p>
        <h2>Sample Response</h2>
        <div class="metrics">
            <pre>` + getSampleJSON() + `</pre>
        </div>
    </div>
</body>
</html>`
	w.Header().Set("Content-Type", "text/html")
	w.Write([]byte(html))
}

func handleMetrics(w http.ResponseWriter, r *http.Request) {
	var resp MetricsResponse

	switch *mode {
	case "down":
		// Bot is down
		resp = MetricsResponse{
			Status:          0,
			Latency:         0,
			ActiveTriangles: 0,
			BestArb:         0,
			PNL:             0,
			Errors:          rng.Intn(20) + 5,
			Timestamp:       time.Now().Unix(),
		}

	case "flap":
		// Randomly flap between ok and down
		if rng.Intn(2) == 0 {
			resp = generateOkMetrics()
		} else {
			resp = MetricsResponse{
				Status:          0,
				Latency:         0,
				ActiveTriangles: 0,
				BestArb:         0,
				PNL:             rng.Intn(2000) - 500,
				Errors:          rng.Intn(15),
				Timestamp:       time.Now().Unix(),
			}
		}

	default: // "ok"
		resp = generateOkMetrics()
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(resp)
}

func generateOkMetrics() MetricsResponse {
	return MetricsResponse{
		Status:          1,
		Latency:         *latencyMs + rng.Intn(20) - 10,
		ActiveTriangles: rng.Intn(15) + 1,
		BestArb:         rng.Intn(80) + 5,  // 0.05% to 0.85%
		PNL:             rng.Intn(5000),    // $0 to $50
		Errors:          rng.Intn(3),       // 0-2 errors
		Timestamp:       time.Now().Unix(),
	}
}

func getSampleJSON() string {
	sample := MetricsResponse{
		Status:          1,
		Latency:         42,
		ActiveTriangles: 5,
		BestArb:         25,
		PNL:             1234,
		Errors:          0,
		Timestamp:       time.Now().Unix(),
	}
	jsonBytes, _ := json.MarshalIndent(sample, "", "  ")
	return string(jsonBytes)
}
