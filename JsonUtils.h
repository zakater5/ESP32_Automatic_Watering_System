#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// Save JSON from a StaticJsonDocument
bool saveJsonToFile(const char* path, const DynamicJsonDocument& doc);

// Save JSON from a String
bool saveJsonToFile(const char* path, const String& jsonString);

// Load JSON into a StaticJsonDocument
bool loadJsonFromFile(const char* path, DynamicJsonDocument& doc);

// Load JSON as a raw String
bool loadJsonAsString(const char* path, String& jsonString);

// Ensure a file exists, create it with defaults if missing
bool ensureJsonFileExists(const char* path, const DynamicJsonDocument& defaults);
