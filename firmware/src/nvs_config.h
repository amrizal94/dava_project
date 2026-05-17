#pragma once
#include <Arduino.h>

struct DeviceConfig {
  String alias;
  String ssid;
  String password;
  bool valid;
};

DeviceConfig nvsLoad();
void nvsSave(const String& alias, const String& ssid, const String& password);
void nvsClear();
