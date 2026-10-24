// secrets.example.h
// Copy this file to secrets.h in the firmware folder you are building and fill in
// THROWAWAY TEST VALUES ONLY. secrets.h is gitignored.
//
// Do NOT put your real home Wi-Fi password here. Create a temporary test SSID
// (phone hotspot or a spare router) for this lab.

#pragma once

// --- Wi-Fi (throwaway test network) ---
#define WIFI_SSID      "LAB_TEST_SSID"
#define WIFI_PASSWORD  "lab-throwaway-pass"

// --- MQTT broker you control (e.g. local Mosquitto) ---
#define MQTT_HOST      "192.168.1.50"   // your broker IP
#define MQTT_PORT      1883              // v1: plaintext. v2 uses 8883 TLS
#define MQTT_USER      "sensor01"
#define MQTT_PASS      "sensor-lab-pass"
#define MQTT_TOPIC     "lab/sensor01/telemetry"

// --- Fake cloud API key (demonstration only) ---
#define CLOUD_API_KEY  "lab_demo_api_key_0000000000"

// --- v1 config page default admin (intentionally weak) ---
#define ADMIN_USER     "admin"
#define ADMIN_PASS     "admin"
