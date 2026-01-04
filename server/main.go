package main

import (
	"encoding/json"
	"log"
	"net/http"
	"time"
)

// Metrics represents the arbitrage bot metrics
type Metrics struct {
	S  float64 `json:"s"`  // spread
	L  float64 `json:"l"`  // liquidity
	A  float64 `json:"a"`  // asset_a
	B  float64 `json:"b"`  // asset_b
	P  float64 `json:"p"`  // profit
	E  float64 `json:"e"`  // efficiency
	TS int64   `json:"ts"` // timestamp
}

func metricsHandler(w http.ResponseWriter, r *http.Request) {
	// Generate test metrics with realistic values
	metrics := Metrics{
		S:  0.0025,      // 0.25% spread
		L:  125000.50,   // liquidity
		A:  50000.25,    // asset A balance
		B:  75000.75,    // asset B balance
		P:  342.18,      // profit
		E:  94.5,        // 94.5% efficiency
		TS: time.Now().Unix(),
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	
	if err := json.NewEncoder(w).Encode(metrics); err != nil {
		log.Printf("Error encoding metrics: %v", err)
		http.Error(w, "Internal server error", http.StatusInternalServerError)
		return
	}
	
	log.Printf("Served metrics: s=%.4f l=%.2f a=%.2f b=%.2f p=%.2f e=%.2f ts=%d", 
		metrics.S, metrics.L, metrics.A, metrics.B, metrics.P, metrics.E, metrics.TS)
}

func main() {
	http.HandleFunc("/api/v1/metrics", metricsHandler)
	
	addr := ":8080"
	log.Printf("Starting ARB Desk test server on %s", addr)
	log.Printf("Metrics endpoint: http://localhost%s/api/v1/metrics", addr)
	
	if err := http.ListenAndServe(addr, nil); err != nil {
		log.Fatalf("Server failed to start: %v", err)
	}
}
