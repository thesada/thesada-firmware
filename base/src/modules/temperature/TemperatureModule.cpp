// thesada-fw - TemperatureModule.cpp
// SPDX-License-Identifier: GPL-3.0-only
#include "TemperatureModule.h"
#include "../../core/Config.h"
#include "../../core/EventBus.h"
#include "../../core/MQTTClient.h"
#include "../../core/Log.h"
#include <LittleFS.h>

static const char* TAG             = "Temp";
static const char* DISCOVERED_PATH = "/discovered_sensors.json";

void TemperatureModule::begin() {
  JsonObject cfg = Config::get();

  _pin          = cfg["temperature"]["pin"]           | 4;
  _intervalMs   = (uint32_t)(cfg["temperature"]["interval_s"] | 60) * 1000;
  _autoDiscover = cfg["temperature"]["auto_discover"] | true;

  _wire    = new OneWire(_pin);
  _sensors = new DallasTemperature(_wire);
  _sensors->begin();

  loadConfigSensors();

  if (_autoDiscover) {
    discoverSensors();
    mergeNames();
    saveDiscovered();
  }

  char msg[64];
  snprintf(msg, sizeof(msg), "Ready - %d sensor(s) on GPIO%d", (int)_sensorList.size(), _pin);
  Log::info(TAG, msg);
}

void TemperatureModule::loop() {
  uint32_t now = millis();
  if (now - _lastRead >= _intervalMs) {
    _lastRead = now;
    readAndPublish();
  }
}

void TemperatureModule::loadConfigSensors() {
  JsonObject cfg = Config::get();
  JsonArray  arr = cfg["temperature"]["sensors"].as<JsonArray>();
  if (arr.isNull()) return;

  for (JsonObject s : arr) {
    const char* addrStr = s["address"] | "";
    const char* sname   = s["name"]    | "";
    if (strlen(addrStr) != 16) continue;

    TempSensor sensor;
    strncpy(sensor.addressStr, addrStr, sizeof(sensor.addressStr) - 1);
    sensor.addressStr[16] = '\0';
    strncpy(sensor.name, (strlen(sname) > 0) ? sname : addrStr, sizeof(sensor.name) - 1);
    sensor.name[sizeof(sensor.name) - 1] = '\0';

    for (int i = 0; i < 8; i++) {
      char byte[3] = { addrStr[i * 2], addrStr[i * 2 + 1], '\0' };
      sensor.address[i] = (uint8_t)strtol(byte, nullptr, 16);
    }

    _sensorList.push_back(sensor);
  }

  char msg[64];
  snprintf(msg, sizeof(msg), "Loaded %d sensor(s) from config", (int)_sensorList.size());
  Log::info(TAG, msg);
}

void TemperatureModule::discoverSensors() {
  int found = _sensors->getDeviceCount();

  char msg[64];
  snprintf(msg, sizeof(msg), "Discovered %d sensor(s) on bus", found);
  Log::info(TAG, msg);

  for (int i = 0; i < found; i++) {
    DeviceAddress addr;
    if (!_sensors->getAddress(addr, i)) continue;

    char addrStr[17];
    addressToStr(addr, addrStr);

    bool exists = false;
    for (auto& s : _sensorList) {
      if (strcmp(s.addressStr, addrStr) == 0) { exists = true; break; }
    }

    if (!exists) {
      TempSensor sensor;
      memcpy(sensor.address, addr, sizeof(DeviceAddress));
      strncpy(sensor.addressStr, addrStr, sizeof(sensor.addressStr) - 1);
      sensor.addressStr[16] = '\0';
      strncpy(sensor.name, addrStr, sizeof(sensor.name) - 1);
      sensor.name[sizeof(sensor.name) - 1] = '\0';
      _sensorList.push_back(sensor);

      char log[64];
      snprintf(log, sizeof(log), "New sensor discovered: %s", addrStr);
      Log::info(TAG, log);
    }
  }
}

void TemperatureModule::mergeNames() {}

void TemperatureModule::saveDiscovered() {
  if (!LittleFS.begin()) return;

  JsonDocument doc;
  JsonArray arr = doc["sensors"].to<JsonArray>();

  for (auto& s : _sensorList) {
    JsonObject obj = arr.add<JsonObject>();
    obj["address"] = s.addressStr;
    obj["name"]    = s.name;
  }

  File f = LittleFS.open(DISCOVERED_PATH, "w");
  if (!f) { Log::error(TAG, "Failed to write discovered_sensors.json"); return; }
  serializeJson(doc, f);
  f.close();
  Log::info(TAG, "Saved discovered sensors to LittleFS");
}

void TemperatureModule::readAndPublish() {
  if (_sensorList.empty()) return;

  _sensors->requestTemperatures();

  JsonDocument doc;
  JsonArray    arr      = doc["sensors"].to<JsonArray>();
  bool         anyValid = false;

  for (auto& s : _sensorList) {
    float temp = _sensors->getTempC(s.address);
    if (temp == DEVICE_DISCONNECTED_C) {
      char log[64];
      snprintf(log, sizeof(log), "Sensor %s disconnected", s.addressStr);
      Log::warn(TAG, log);
      continue;
    }
    JsonObject obj = arr.add<JsonObject>();
    obj["name"]    = s.name;
    obj["address"] = s.addressStr;
    obj["temp_c"]  = serialized(String(temp, 2));
    anyValid = true;
  }

  if (!anyValid) return;

  JsonObject cfg = Config::get();
  const char* prefix = cfg["mqtt"]["topic_prefix"] | "thesada/node";
  char topic[64];
  snprintf(topic, sizeof(topic), "%s/sensor/temperature", prefix);

  char payload[512];
  serializeJson(doc, payload, sizeof(payload));
  MQTTClient::publish(topic, payload);

  EventBus::publish("temperature", doc.as<JsonObject>());
}

void TemperatureModule::addressToStr(DeviceAddress addr, char* out) {
  for (int i = 0; i < 8; i++) snprintf(out + (i * 2), 3, "%02X", addr[i]);
  out[16] = '\0';
}
