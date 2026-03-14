// thesada-fw - WiFiManager.h
// WiFi with exponential backoff reconnect and fallback AP
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <Arduino.h>

class WiFiManager {
public:
  static void begin();
  static void loop();
  static bool connected();

private:
  static void connect();
  static void startFallbackAP();

  static bool     _apActive;
  static uint32_t _lastAttempt;
  static uint32_t _retryInterval;
  static uint8_t  _retryCount;

  static constexpr uint32_t RETRY_MIN_MS      = 2000;
  static constexpr uint32_t RETRY_MAX_MS      = 60000;
  static constexpr uint8_t  CONNECT_TIMEOUT_S = 15;
};
