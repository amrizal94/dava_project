#include "nvs_config.h"
#include <Preferences.h>

static Preferences prefs;

DeviceConfig nvsLoad() {
  prefs.begin("dava", true);
  DeviceConfig cfg;
  cfg.alias    = prefs.getString("alias", "");
  cfg.ssid     = prefs.getString("ssid", "");
  cfg.password = prefs.getString("pass", "");
  cfg.valid    = cfg.ssid.length() > 0;
  prefs.end();
  return cfg;
}

void nvsSave(const String& alias, const String& ssid, const String& password) {
  prefs.begin("dava", false);
  prefs.putString("alias", alias);
  prefs.putString("ssid",  ssid);
  prefs.putString("pass",  password);
  prefs.end();
}

void nvsClear() {
  prefs.begin("dava", false);
  prefs.clear();
  prefs.end();
}
