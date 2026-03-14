// thesada-fw - PWMModule.cpp
// SPDX-License-Identifier: GPL-3.0-only
#include "PWMModule.h"
#include "../../core/Config.h"
#include "../../core/EventBus.h"
#include "../../core/MQTTClient.h"
#include "../../core/Log.h"

static const char* TAG = "PWM";

uint8_t  PWMModule::_pin          = 16;
uint32_t PWMModule::_frequency    = 25000;
uint8_t  PWMModule::_resolution   = 8;
float    PWMModule::_currentLevel = 0.0f;

void PWMModule::begin() {
  JsonObject cfg = Config::get();

  _pin        = cfg["pwm"]["pin"]          | 16;
  _frequency  = cfg["pwm"]["frequency_hz"] | 25000;
  _resolution = cfg["pwm"]["resolution"]   | 8;

  ledcAttach(_pin, _frequency, _resolution);
  ledcWrite(_pin, 0);

  char msg[64];
  snprintf(msg, sizeof(msg), "Ready - GPIO%d, %luHz, %d-bit", _pin, _frequency, _resolution);
  Log::info(TAG, msg);

  subscribeEventBus();
  subscribeMQTT();
}

void PWMModule::loop() {}

void PWMModule::setLevel(float level) {
  level = constrain(level, 0.0f, 1.0f);
  _currentLevel = level;

  uint32_t maxVal = (1 << _resolution) - 1;
  uint32_t duty   = (uint32_t)(level * maxVal);
  ledcWrite(_pin, duty);

  char msg[64];
  snprintf(msg, sizeof(msg), "Level set to %.0f%% (duty %lu/%lu)", level * 100, duty, maxVal);
  Log::info(TAG, msg);

  JsonObject  cfg    = Config::get();
  const char* prefix = cfg["mqtt"]["topic_prefix"] | "thesada/node";
  char topic[64];
  snprintf(topic, sizeof(topic), "%s/sensor/pwm", prefix);

  char payload[64];
  snprintf(payload, sizeof(payload), "{\"level\":%.2f,\"percent\":%.0f}", level, level * 100);
  MQTTClient::publish(topic, payload);
}

void PWMModule::subscribeEventBus() {
  EventBus::subscribe("pwm_set", [](JsonObject data) {
    float level = data["level"] | -1.0f;
    if (level < 0.0f || level > 1.0f) {
      Log::warn(TAG, "Invalid level in pwm_set event - expected 0.0-1.0");
      return;
    }
    PWMModule::setLevel(level);
  });
}

void PWMModule::subscribeMQTT() {
  Log::info(TAG, "MQTT cmd topic ready (requires MQTTClient subscription dispatch)");
}

void PWMModule::onMQTTCommand(const char* payload) {
  int percent = atoi(payload);
  if (percent < 0 || percent > 100) {
    Log::warn(TAG, "Invalid PWM percent - expected 0-100");
    return;
  }
  setLevel(percent / 100.0f);
}
