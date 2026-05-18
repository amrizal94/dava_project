#include "sensors.h"
#include "config.h"

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <BH1750.h>
#include <PZEM004Tv30.h>

static OneWire         owIndoor(ONE_WIRE_INDOOR_PIN);
static OneWire         owOutdoor(ONE_WIRE_OUTDOOR_PIN);
static DallasTemperature dsIndoor(&owIndoor);
static DallasTemperature dsOutdoor(&owOutdoor);
static BH1750          bh1750;
// PZEM-004T on Serial2 (RX2=16, TX2=17 by default)
static PZEM004Tv30     pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);

static inline bool isTempValid(float t) {
  return !isnan(t) && t != DEVICE_DISCONNECTED_C && t != 85.0f && t > -55.0f && t < 125.0f;
}

void sensorsInit() {
  dsIndoor.begin();
  dsOutdoor.begin();

  Wire.begin();
  bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  pinMode(POWER_STATUS_PIN, INPUT);

  Serial.println("[Sensors] Init done.");
}

SensorData sensorsRead() {
  SensorData d{};

  // DS18B20 — blocking read (~750ms per sensor)
  dsIndoor.requestTemperatures();
  float ti = dsIndoor.getTempCByIndex(0);
  d.tempIndoor = isTempValid(ti) ? ti : NAN;

  dsOutdoor.requestTemperatures();
  float to = dsOutdoor.getTempCByIndex(0);
  d.tempOutdoor = isTempValid(to) ? to : NAN;

  // BH1750
  d.lux = bh1750.readLightLevel();

  // PZEM-004T
  d.voltage     = pzem.voltage();
  d.current     = pzem.current();
  d.power       = pzem.power();
  d.frequency   = pzem.frequency();
  d.powerFactor = pzem.pf();

  // Power status relay
  d.powerStatus = digitalRead(POWER_STATUS_PIN) == HIGH;

  d.valid = true;
  return d;
}
