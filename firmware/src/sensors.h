#pragma once

struct SensorData {
  float tempIndoor;
  float tempOutdoor;
  float lux;
  float voltage;
  float current;
  float power;
  float frequency;
  float powerFactor;
  bool  powerStatus;
  bool  valid;
};

void sensorsInit();
SensorData sensorsRead();
