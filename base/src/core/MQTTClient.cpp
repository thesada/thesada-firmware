// thesada-fw - MQTTClient.cpp
// SPDX-License-Identifier: GPL-3.0-only
#include "MQTTClient.h"
#include "Config.h"
#include "WiFiManager.h"
#include "Log.h"

static const char* TAG = "MQTT";

WiFiClient   MQTTClient::_wifiClient;
PubSubClient MQTTClient::_client(_wifiClient);

MQTTMessage  MQTTClient::_queue[MQTT_QUEUE_SIZE];
uint8_t      MQTTClient::_queueHead     = 0;
uint8_t      MQTTClient::_queueTail     = 0;
uint8_t      MQTTClient::_queueCount    = 0;
uint32_t     MQTTClient::_lastAttempt   = 0;
uint32_t     MQTTClient::_retryInterval = RETRY_MIN_MS;
uint8_t      MQTTClient::_retryCount    = 0;

void MQTTClient::begin() {
  JsonObject  cfg  = Config::get();
  const char* host = cfg["mqtt"]["broker"] | "";
  uint16_t    port = cfg["mqtt"]["port"]   | 1883;

  if (strlen(host) == 0) {
    Log::error(TAG, "No broker in config");
    return;
  }

  _client.setServer(host, port);
  _client.setKeepAlive(60);

  // TODO: when MQTT_TLS is enabled, swap _wifiClient for WiFiClientSecure
  // and load ISRG Root X1 cert via client.setCACert(...)

  connect();
}

void MQTTClient::connect() {
  if (!WiFiManager::connected()) return;

  JsonObject  cfg      = Config::get();
  const char* clientId = cfg["device"]["name"]   | "thesada-node";
  const char* user     = cfg["mqtt"]["user"]      | "";
  const char* password = cfg["mqtt"]["password"]  | "";

  char msg[64];
  snprintf(msg, sizeof(msg), "Connecting as %s...", clientId);
  Log::info(TAG, msg);

  bool ok = (strlen(user) > 0)
    ? _client.connect(clientId, user, password)
    : _client.connect(clientId);

  if (ok) {
    _retryInterval = RETRY_MIN_MS;
    _retryCount    = 0;
    Log::info(TAG, "Connected");
    flushQueue();
  } else {
    char err[64];
    snprintf(err, sizeof(err), "Failed (rc=%d) - retry in %lums",
             _client.state(), _retryInterval);
    Log::warn(TAG, err);
    _retryCount++;
    _retryInterval = min(_retryInterval * 2, (uint32_t)RETRY_MAX_MS);
  }
}

void MQTTClient::loop() {
  if (_client.connected()) {
    _client.loop();
    return;
  }

  if (!WiFiManager::connected()) return;

  uint32_t now = millis();
  if (now - _lastAttempt >= _retryInterval) {
    _lastAttempt = now;
    connect();
  }
}

void MQTTClient::publish(const char* topic, const char* payload) {
  if (_client.connected()) {
    _client.publish(topic, payload);
    return;
  }
  enqueue(topic, payload);
}

void MQTTClient::enqueue(const char* topic, const char* payload) {
  if (_queueCount == MQTT_QUEUE_SIZE) {
    Log::warn(TAG, "Queue full - dropping oldest message");
    _queueHead = (_queueHead + 1) % MQTT_QUEUE_SIZE;
    _queueCount--;
  }

  MQTTMessage& msg = _queue[_queueTail];
  strncpy(msg.topic,   topic,   sizeof(msg.topic)   - 1);
  strncpy(msg.payload, payload, sizeof(msg.payload) - 1);
  msg.topic[sizeof(msg.topic)     - 1] = '\0';
  msg.payload[sizeof(msg.payload) - 1] = '\0';
  msg.valid = true;

  _queueTail = (_queueTail + 1) % MQTT_QUEUE_SIZE;
  _queueCount++;

  char log[64];
  snprintf(log, sizeof(log), "Queued (%d/%d): %s", _queueCount, MQTT_QUEUE_SIZE, topic);
  Log::info(TAG, log);
}

void MQTTClient::flushQueue() {
  while (_queueCount > 0) {
    MQTTMessage& msg = _queue[_queueHead];
    if (msg.valid) {
      _client.publish(msg.topic, msg.payload);
      msg.valid = false;

      char log[64];
      snprintf(log, sizeof(log), "Flushed queued: %s", msg.topic);
      Log::info(TAG, log);
    }
    _queueHead = (_queueHead + 1) % MQTT_QUEUE_SIZE;
    _queueCount--;
  }
}

bool MQTTClient::connected() {
  return _client.connected();
}
