#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "nvs_config.h"
#include "wifi_manager.h"
#include "sensors.h"
#include "mqtt_client.h"

// ── Light state ───────────────────────────────────────────────
static String lightMode = "auto";
static int    lightBrightness = 100;

void onLightCommand(const String& mode, int brightness) {
  lightMode = mode;
  if (mode == "manual") {
    lightBrightness = constrain(brightness, 1, 100);
    int duty = map(lightBrightness, 1, 100, 3, 255);
    ledcWrite(LIGHT_PWM_CHANNEL, duty);
    Serial.printf("[Light] Manual %d%%\n", lightBrightness);
  } else {
    // Auto mode: device manages light internally (e.g., based on lux)
    // Placeholder — customise as needed
    ledcWrite(LIGHT_PWM_CHANNEL, 255);
    Serial.println("[Light] Auto mode");
  }
}

// ── Helper: MAC string ────────────────────────────────────────
static String getMac() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  // PWM for light
  ledcSetup(LIGHT_PWM_CHANNEL, LIGHT_PWM_FREQ, LIGHT_PWM_RES);
  ledcAttachPin(LIGHT_PWM_PIN, LIGHT_PWM_CHANNEL);
  ledcWrite(LIGHT_PWM_CHANNEL, 0);

  // Load NVS config
  DeviceConfig cfg = nvsLoad();

  String mac = getMac();
  String apName = String(AP_PREFIX) + mac.substring(9);  // last 8 chars: XX:XX:XX

  if (!cfg.valid) {
    Serial.println("[Boot] No config found, starting provisioning...");
    wifiProvisioningStart(apName);
    // Does not return — reboots after save
  }

  // Connect to WiFi
  Serial.printf("[WiFi] Connecting to %s\n", cfg.ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.ssid.c_str(), cfg.password.c_str());

  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Failed to connect, starting provisioning...");
    nvsClear();
    wifiProvisioningStart(apName);
  }

  Serial.println("[WiFi] Connected: " + WiFi.localIP().toString());

  // Init sensors
  sensorsInit();

  // Init MQTT
  mqttInit(mac, cfg.alias);
}

// ── Loop ──────────────────────────────────────────────────────
static unsigned long lastTelemetry = 0;
static unsigned long lastStatus    = 0;

void loop() {
  mqttLoop();

  unsigned long now = millis();

  if (now - lastTelemetry >= TELEMETRY_INTERVAL_MS) {
    lastTelemetry = now;
    SensorData d = sensorsRead();
    mqttPublishTelemetry(d);
  }

  if (now - lastStatus >= STATUS_INTERVAL_MS) {
    lastStatus = now;
    mqttPublishStatus();
  }

  delay(10);
}
