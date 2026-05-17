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

  // DS18B20 indoor
  dsIndoor.requestTemperatures();
  d.tempIndoor = dsIndoor.getTempCByIndex(0);
  if (d.tempIndoor == DEVICE_DISCONNECTED_C) d.tempIndoor = NAN;

  // DS18B20 outdoor
  dsOutdoor.requestTemperatures();
  d.tempOutdoor = dsOutdoor.getTempCByIndex(0);
  if (d.tempOutdoor == DEVICE_DISCONNECTED_C) d.tempOutdoor = NAN;

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
