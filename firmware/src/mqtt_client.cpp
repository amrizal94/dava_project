#include "mqtt_client.h"
#include "config.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

static WiFiClient   wifiClient;
static PubSubClient mqtt(wifiClient);

static String _mac;
static String _alias;

static String topicTelemetry;
static String topicStatus;
static String topicRegister;
static String topicControlLight;
static String topicAckLight;

static void mqttCallback(char* topic, byte* payload, unsigned int len) {
  String t(topic);
  if (t != topicControlLight) return;

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, payload, len)) return;

  String mode = doc["mode"] | "auto";
  int brightness = doc["brightness"] | 100;
  onLightCommand(mode, brightness);

  // Send ack
  StaticJsonDocument<64> ack;
  ack["mode"] = mode;
  ack["brightness"] = brightness;
  String ackStr;
  serializeJson(ack, ackStr);
  mqtt.publish(topicAckLight.c_str(), ackStr.c_str());
}

void mqttInit(const String& mac, const String& alias) {
  _mac   = mac;
  _alias = alias;

  topicTelemetry    = "dava/" + mac + "/telemetry";
  topicStatus       = "dava/" + mac + "/status";
  topicRegister     = "dava/" + mac + "/register";
  topicControlLight = "dava/" + mac + "/control/light";
  topicAckLight     = "dava/" + mac + "/ack/light";

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setKeepAlive(60);
  mqtt.setBufferSize(512);
}

static void mqttConnect() {
  String clientId = "dava-" + _mac;
  Serial.print("[MQTT] Connecting... ");

  bool ok;
  if (strlen(MQTT_USERNAME) > 0) {
    ok = mqtt.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD);
  } else {
    ok = mqtt.connect(clientId.c_str());
  }

  if (!ok) {
    Serial.println("failed, rc=" + String(mqtt.state()));
    return;
  }

  Serial.println("connected");
  mqtt.subscribe(topicControlLight.c_str(), 1);

  // Register device
  StaticJsonDocument<128> reg;
  reg["alias"]    = _alias;
  reg["firmware"] = "1.0.0";
  String regStr;
  serializeJson(reg, regStr);
  mqtt.publish(topicRegister.c_str(), regStr.c_str(), true);
}

void mqttLoop() {
  if (!mqtt.connected()) {
    mqttConnect();
  }
  mqtt.loop();
}

bool mqttConnected() { return mqtt.connected(); }

void mqttPublishTelemetry(const SensorData& d) {
  StaticJsonDocument<256> doc;
  doc["temp_in"]      = isnan(d.tempIndoor)  ? nullptr : JsonVariant(d.tempIndoor);
  doc["temp_out"]     = isnan(d.tempOutdoor) ? nullptr : JsonVariant(d.tempOutdoor);
  doc["lux"]          = isnan(d.lux)         ? nullptr : JsonVariant(d.lux);
  doc["voltage"]      = isnan(d.voltage)     ? nullptr : JsonVariant(d.voltage);
  doc["current"]      = isnan(d.current)     ? nullptr : JsonVariant(d.current);
  doc["power"]        = isnan(d.power)       ? nullptr : JsonVariant(d.power);
  doc["frequency"]    = isnan(d.frequency)   ? nullptr : JsonVariant(d.frequency);
  doc["power_factor"] = isnan(d.powerFactor) ? nullptr : JsonVariant(d.powerFactor);
  doc["power_status"] = d.powerStatus;

  String out;
  serializeJson(doc, out);
  mqtt.publish(topicTelemetry.c_str(), out.c_str());
}

void mqttPublishStatus() {
  StaticJsonDocument<32> doc;
  doc["uptime"] = millis() / 1000;
  String out;
  serializeJson(doc, out);
  mqtt.publish(topicStatus.c_str(), out.c_str());
}
