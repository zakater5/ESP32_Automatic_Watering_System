#include "AutomationRules.h"
#include <ArduinoJson.h>
#include <TimeLib.h> // for hour(), minute(), now()
#include <LittleFS.h>

bool loadAutomationRules(std::vector<AutomationRule>& rules, const char* path) {
    rules.clear();

    DynamicJsonDocument doc(4096);
    if (!loadJsonFromFile(path, doc)) {
        Serial.println("Failed to load rules.json");
        return false;
    }

    if (!doc.is<JsonArray>()) {
        Serial.println("rules.json is not an array");
        return false;
    }

    for (JsonObject obj : doc.as<JsonArray>()) {
        AutomationRule r;
        r.id = obj["id"] | 0;
        r.sensor = obj["sensor"] | "";
        r.op = obj["operator"] | "";
        r.value = obj["value"] | 0;
        r.action = obj["actionSelect"] | "";
        if (obj.containsKey("timeRange")) {
            r.timeStart = obj["timeRange"]["start"] | "";
            r.timeEnd = obj["timeRange"]["end"] | "";
        }
        r.enabled = obj["enabled"] | true;
        rules.push_back(r);
    }

    Serial.printf("Loaded %d automation rules\n", rules.size());
    return true;
}

bool isWithinTimeRange(const String& start, const String& end) {
    if (start == "" || end == "") return true;

    int nowH = hour(now());
    int nowM = minute(now());

    int startH = start.substring(0,2).toInt();
    int startM = start.substring(3,5).toInt();
    int endH   = end.substring(0,2).toInt();
    int endM   = end.substring(3,5).toInt();

    int nowTotal = nowH*60 + nowM;
    int startTotal = startH*60 + startM;
    int endTotal = endH*60 + endM;

    return nowTotal >= startTotal && nowTotal <= endTotal;
}

void evaluateAutomationRules(const std::vector<AutomationRule>& rules,
                             float temperature, float humidity,
                             int moisture, int light,
                             int pumpPin) {
    for (const auto& rule : rules) {
        if (!rule.enabled) continue;

        float sensorValue = 0;
        if (rule.sensor == "temperature") sensorValue = temperature;
        else if (rule.sensor == "humidity") sensorValue = humidity;
        else if (rule.sensor == "moisture") sensorValue = moisture;
        else if (rule.sensor == "light") sensorValue = light;
        else continue;

        bool conditionMet = false;
        if (rule.op == ">") conditionMet = sensorValue > rule.value;
        else if (rule.op == "<") conditionMet = sensorValue < rule.value;
        else if (rule.op == "=") conditionMet = sensorValue == rule.value;

        if (conditionMet && isWithinTimeRange(rule.timeStart, rule.timeEnd)) {
            if (rule.action == "pump_on") digitalWrite(pumpPin, HIGH);
            else if (rule.action == "pump_off") digitalWrite(pumpPin, LOW);
        }
    }
}
