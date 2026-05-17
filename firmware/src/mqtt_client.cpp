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

  JsonDocument doc;
  if (deserializeJson(doc, payload, len)) return;

  String mode = doc["mode"] | "auto";
  int brightness = doc["brightness"] | 100;
  onLightCommand(mode, brightness);

  JsonDocument ack;
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

  JsonDocument reg;
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

static void setFloat(JsonDocument& doc, const char* key, float val) {
  if (isnan(val)) doc[key] = nullptr;
  else doc[key] = val;
}

void mqttPublishTelemetry(const SensorData& d) {
  JsonDocument doc;
  setFloat(doc, "temp_in",      d.tempIndoor);
  setFloat(doc, "temp_out",     d.tempOutdoor);
  setFloat(doc, "lux",          d.lux);
  setFloat(doc, "voltage",      d.voltage);
  setFloat(doc, "current",      d.current);
  setFloat(doc, "power",        d.power);
  setFloat(doc, "frequency",    d.frequency);
  setFloat(doc, "power_factor", d.powerFactor);
  doc["power_status"] = d.powerStatus;

  String out;
  serializeJson(doc, out);
  mqtt.publish(topicTelemetry.c_str(), out.c_str());
}

void mqttPublishStatus() {
  JsonDocument doc;
  doc["uptime"] = millis() / 1000;
  String out;
  serializeJson(doc, out);
  mqtt.publish(topicStatus.c_str(), out.c_str());
}
