// thesada-fw - WiFiManager.cpp
// SPDX-License-Identifier: GPL-3.0-only
#include "WiFiManager.h"
#include "Config.h"
#include "Log.h"
#include <WiFi.h>

static const char* TAG = "WiFi";

bool     WiFiManager::_apActive      = false;
uint32_t WiFiManager::_lastAttempt   = 0;
uint32_t WiFiManager::_retryInterval = RETRY_MIN_MS;
uint8_t  WiFiManager::_retryCount    = 0;

void WiFiManager::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  connect();
}

void WiFiManager::connect() {
  JsonObject  cfg      = Config::get();
  const char* ssid     = cfg["wifi"]["ssid"]     | "";
  const char* password = cfg["wifi"]["password"] | "";

  if (strlen(ssid) == 0) {
    Log::error(TAG, "No SSID in config - starting fallback AP");
    startFallbackAP();
    return;
  }

  Log::info(TAG, "Connecting to WiFi...");
  WiFi.begin(ssid, password);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start >= CONNECT_TIMEOUT_S * 1000UL) {
      Log::warn(TAG, "Connection timed out - starting fallback AP");
      startFallbackAP();
      return;
    }
    delay(250);
  }

  _apActive      = false;
  _retryInterval = RETRY_MIN_MS;
  _retryCount    = 0;

  char msg[64];
  snprintf(msg, sizeof(msg), "Connected - IP: %s", WiFi.localIP().toString().c_str());
  Log::info(TAG, msg);
}

void WiFiManager::startFallbackAP() {
  JsonObject  cfg  = Config::get();
  const char* name = cfg["device"]["name"] | "";

  char apSSID[32];
  if (strlen(name) > 0) {
    snprintf(apSSID, sizeof(apSSID), "%s-setup", name);
  } else {
    // fallback if config not available - use chip ID
    snprintf(apSSID, sizeof(apSSID), "thesada-%06llX", (unsigned long long)ESP.getEfuseMac());
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID);
  _apActive = true;

  char msg[64];
  snprintf(msg, sizeof(msg), "Fallback AP started: %s", apSSID);
  Log::warn(TAG, msg);
}

void WiFiManager::loop() {
  if (_apActive) return;

  if (WiFi.status() == WL_CONNECTED) {
    _retryInterval = RETRY_MIN_MS;
    _retryCount    = 0;
    return;
  }

  uint32_t now = millis();
  if (now - _lastAttempt >= _retryInterval) {
    _lastAttempt = now;

    char msg[64];
    snprintf(msg, sizeof(msg), "Reconnecting (attempt %d, interval %lums)...",
             _retryCount + 1, _retryInterval);
    Log::warn(TAG, msg);

    connect();
    _retryCount++;
    _retryInterval = min(_retryInterval * 2, (uint32_t)RETRY_MAX_MS);
  }
}

bool WiFiManager::connected() {
  return WiFi.status() == WL_CONNECTED;
}
