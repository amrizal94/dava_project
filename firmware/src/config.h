#pragma once

// ── MQTT ──────────────────────────────────────────────────────
#define MQTT_HOST       "45.66.153.156"
#define MQTT_PORT       1883
#define MQTT_USERNAME   ""
#define MQTT_PASSWORD   ""

// ── Telemetry interval ────────────────────────────────────────
#define TELEMETRY_INTERVAL_MS  10000UL   // 10 detik
#define STATUS_INTERVAL_MS     30000UL   // 30 detik

// ── Pin assignments (ESP32 DevKit V1) ─────────────────────────
// DS18B20 indoor
#define ONE_WIRE_INDOOR_PIN    4

// DS18B20 outdoor
#define ONE_WIRE_OUTDOOR_PIN   5

// BH1750 — I2C default: SDA=21, SCL=22

// PZEM-004T — Serial2 default: RX2=16, TX2=17
#define PZEM_RX_PIN   16
#define PZEM_TX_PIN   17

// Relay / power status input (HIGH = ON)
#define POWER_STATUS_PIN  18

// Light PWM output (dimmer control)
#define LIGHT_PWM_PIN     19
#define LIGHT_PWM_CHANNEL  0
#define LIGHT_PWM_FREQ    5000
#define LIGHT_PWM_RES     8      // 8-bit = 0-255

// ── WiFi provisioning AP ──────────────────────────────────────
#define AP_PREFIX       "DAVA-Setup-"
#define AP_PASSWORD     ""           // kosong = open network
#define PORTAL_TIMEOUT_MS  300000UL  // 5 menit, lalu reboot
