// thesada-fw - MQTTClient.h
// MQTT with exponential backoff reconnect and ring buffer publish queue
// TLS-ready: set MQTT_TLS true in config.h to enable
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

static constexpr uint8_t MQTT_QUEUE_SIZE = 8;

struct MQTTMessage {
  char topic[64];
  char payload[256];
  bool valid;
};

class MQTTClient {
public:
  static void begin();
  static void loop();
  static void publish(const char* topic, const char* payload);
  static bool connected();

private:
  static void connect();
  static void flushQueue();
  static void enqueue(const char* topic, const char* payload);

  static WiFiClient    _wifiClient;
  static PubSubClient  _client;

  static MQTTMessage   _queue[MQTT_QUEUE_SIZE];
  static uint8_t       _queueHead;
  static uint8_t       _queueTail;
  static uint8_t       _queueCount;

  static uint32_t      _lastAttempt;
  static uint32_t      _retryInterval;
  static uint8_t       _retryCount;

  static constexpr uint32_t RETRY_MIN_MS = 2000;
  static constexpr uint32_t RETRY_MAX_MS = 60000;
};
