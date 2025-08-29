#include "JsonUtils.h"

// Save JSON from a StaticJsonDocument
bool saveJsonToFile(const char* path, const DynamicJsonDocument& doc) {
  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.printf("Failed to open %s for writing\n", path);
    return false;
  }
  if (serializeJson(doc, file) == 0) {
    Serial.printf("Failed to write JSON to %s\n", path);
    file.close();
    return false;
  }
  file.close();
  Serial.printf("JSON saved successfully to %s\n", path);
  return true;
}

// Save JSON from a String
bool saveJsonToFile(const char* path, const String& jsonString) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
    Serial.printf("Invalid JSON string for %s: %s\n", path, error.c_str());
    return false;
  }
  return saveJsonToFile(path, doc);
}

// Load JSON into a StaticJsonDocument
bool loadJsonFromFile(const char* path, DynamicJsonDocument& doc) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.printf("Failed to open %s for reading\n", path);
    return false;
  }
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
    Serial.printf("Failed to parse JSON from %s: %s\n", path, error.c_str());
    return false;
  }
  Serial.printf("JSON loaded successfully from %s\n", path);
  return true;
}

// Load JSON as a raw String
bool loadJsonAsString(const char* path, String& jsonString) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.printf("Failed to open %s for reading\n", path);
    return false;
  }
  jsonString = file.readString();
  file.close();
  return true;
}

// Ensure a file exists, create with defaults if missing
bool ensureJsonFileExists(const char* path, const DynamicJsonDocument& defaults) {
  if (!LittleFS.exists(path)) {
    Serial.printf("%s not found, creating default...\n", path);
    return saveJsonToFile(path, defaults);
  }
  return true;
}
