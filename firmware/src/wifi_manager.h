#pragma once
#include <Arduino.h>

// Buka AP + captive portal, tunggu sampai user simpan konfigurasi.
// Setelah berhasil, simpan ke NVS dan reboot.
void wifiProvisioningStart(const String& apName);
