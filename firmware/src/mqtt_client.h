#pragma once
#include <Arduino.h>
#include "sensors.h"

void mqttInit(const String& mac, const String& alias);
void mqttLoop();
bool mqttConnected();
void mqttPublishTelemetry(const SensorData& d);
void mqttPublishStatus();

// Callback saat server kirim perintah lampu
// mode: "auto" atau "manual", brightness: 1-100 (hanya saat manual)
extern void onLightCommand(const String& mode, int brightness);
