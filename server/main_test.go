package main

import (
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"
)

func TestMetricsHandler(t *testing.T) {
	req, err := http.NewRequest("GET", "/api/v1/metrics", nil)
	if err != nil {
		t.Fatal(err)
	}

	rr := httptest.NewRecorder()
	handler := http.HandlerFunc(metricsHandler)
	handler.ServeHTTP(rr, req)

	// Check status code
	if status := rr.Code; status != http.StatusOK {
		t.Errorf("handler returned wrong status code: got %v want %v",
			status, http.StatusOK)
	}

	// Check Content-Type
	contentType := rr.Header().Get("Content-Type")
	if contentType != "application/json" {
		t.Errorf("handler returned wrong content type: got %v want %v",
			contentType, "application/json")
	}

	// Parse response
	var metrics Metrics
	if err := json.NewDecoder(rr.Body).Decode(&metrics); err != nil {
		t.Fatalf("Failed to decode response: %v", err)
	}

	// Verify all required fields are present and valid
	if metrics.S == 0 {
		t.Error("Spread (s) should not be zero")
	}
	if metrics.L == 0 {
		t.Error("Liquidity (l) should not be zero")
	}
	if metrics.A == 0 {
		t.Error("Asset A (a) should not be zero")
	}
	if metrics.B == 0 {
		t.Error("Asset B (b) should not be zero")
	}
	if metrics.TS == 0 {
		t.Error("Timestamp (ts) should not be zero")
	}

	// Verify response is compact (< 256 bytes)
	responseSize := rr.Body.Len()
	if responseSize >= 256 {
		t.Errorf("Response size %d bytes exceeds 256 byte limit", responseSize)
	}

	t.Logf("Metrics response: s=%.4f l=%.2f a=%.2f b=%.2f p=%.2f e=%.2f ts=%d (size: %d bytes)",
		metrics.S, metrics.L, metrics.A, metrics.B, metrics.P, metrics.E, metrics.TS, responseSize)
}

func TestMetricsJSONStructure(t *testing.T) {
	req, err := http.NewRequest("GET", "/api/v1/metrics", nil)
	if err != nil {
		t.Fatal(err)
	}

	rr := httptest.NewRecorder()
	handler := http.HandlerFunc(metricsHandler)
	handler.ServeHTTP(rr, req)

	// Parse as generic map to verify exact fields
	var data map[string]interface{}
	if err := json.NewDecoder(rr.Body).Decode(&data); err != nil {
		t.Fatalf("Failed to decode response: %v", err)
	}

	// Check that all required fields exist
	requiredFields := []string{"s", "l", "a", "b", "p", "e", "ts"}
	for _, field := range requiredFields {
		if _, exists := data[field]; !exists {
			t.Errorf("Required field '%s' missing from response", field)
		}
	}

	// Verify all values are numbers
	for key, value := range data {
		switch value.(type) {
		case float64, int64:
			// OK - numeric type
		default:
			t.Errorf("Field '%s' has non-numeric value: %v (type: %T)", key, value, value)
		}
	}
}
