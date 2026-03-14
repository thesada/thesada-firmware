// thesada-fw - TemperatureModule.h
// DS18B20 1-Wire temperature sensing
// Auto-discovers sensors on boot, merges with config.json names
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "../../core/Module.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <vector>
#include <string>

struct TempSensor {
  DeviceAddress address;
  char          addressStr[17];
  char          name[32];
};

class TemperatureModule : public Module {
public:
  void begin() override;
  void loop() override;
  const char* name() override { return "TemperatureModule"; }

private:
  void        discoverSensors();
  void        loadConfigSensors();
  void        mergeNames();
  void        saveDiscovered();
  void        readAndPublish();
  void        addressToStr(DeviceAddress addr, char* out);
  std::string buildTopic();

  OneWire*           _wire    = nullptr;
  DallasTemperature* _sensors = nullptr;

  std::vector<TempSensor> _sensorList;

  uint32_t _lastRead      = 0;
  uint32_t _intervalMs    = 60000;
  bool     _autoDiscover  = true;
  uint8_t  _pin           = 4;
};
