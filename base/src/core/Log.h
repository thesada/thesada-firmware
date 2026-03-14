// thesada-fw - Log.h
// Shared logging - Serial now, MQTT later
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <Arduino.h>

enum class LogLevel {
  DEBUG,
  INFO,
  WARN,
  ERROR
};

class Log {
public:
  static void debug(const char* tag, const char* msg);
  static void info(const char* tag, const char* msg);
  static void warn(const char* tag, const char* msg);
  static void error(const char* tag, const char* msg);

private:
  static void write(LogLevel level, const char* tag, const char* msg);
  static const char* levelStr(LogLevel level);
};
