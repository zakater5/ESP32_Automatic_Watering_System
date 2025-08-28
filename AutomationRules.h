#pragma once
#include <Arduino.h>
#include <vector>
#include "JsonUtils.h"

struct AutomationRule {
    unsigned long id;
    String sensor;
    String op;          // ">", "<", "="
    float value;
    String action;      // "pump_on" or "pump_off"
    String timeStart;   // optional "HH:MM"
    String timeEnd;     // optional "HH:MM"
    bool enabled;
};

// Load rules from LittleFS JSON file
bool loadAutomationRules(std::vector<AutomationRule>& rules, const char* path = "/rules.json");

// Evaluate all rules and perform actions on pins
void evaluateAutomationRules(const std::vector<AutomationRule>& rules,
                             float temperature, float humidity,
                             int moisture, int light,
                             int pumpPin);

// Check if current time is within a rule's time range
bool isWithinTimeRange(const String& start, const String& end);
