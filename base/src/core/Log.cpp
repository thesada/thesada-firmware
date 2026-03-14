// thesada-fw - Log.cpp
// SPDX-License-Identifier: GPL-3.0-only
#include "Log.h"

void Log::write(LogLevel level, const char* tag, const char* msg) {
  Serial.printf("[%s][%s] %s\n", levelStr(level), tag, msg);
}

const char* Log::levelStr(LogLevel level) {
  switch (level) {
    case LogLevel::DEBUG: return "DBG";
    case LogLevel::INFO:  return "INF";
    case LogLevel::WARN:  return "WRN";
    case LogLevel::ERROR: return "ERR";
    default:              return "???";
  }
}

void Log::debug(const char* tag, const char* msg) { write(LogLevel::DEBUG, tag, msg); }
void Log::info(const char* tag, const char* msg)  { write(LogLevel::INFO,  tag, msg); }
void Log::warn(const char* tag, const char* msg)  { write(LogLevel::WARN,  tag, msg); }
void Log::error(const char* tag, const char* msg) { write(LogLevel::ERROR, tag, msg); }
