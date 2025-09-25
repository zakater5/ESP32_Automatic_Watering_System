#include <LittleFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "JsonUtils.h"
#include "AutomationRules.h"
#include "Pins.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
DynamicJsonDocument doc(1024);
#pragma GCC diagnostic pop

std::vector<AutomationRule> rules;

bool reset_settings = false; // Če vklopjeno se resetira vsa konfiguracija nazaj na prvotno ob zagonu

// display:
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// button:
bool lcdOn = true;

DHT dht(DHTPIN, DHTTYPE);
float temperature = 0.0; // spremenljivka v katero se piše temperatura
float humidity = 0.0; // spremenljivka v katero se piše vlaga ozračja

int moistureLevel_1 = 0;
int moistureLevel_2 = 0;
int moistureLevel_3 = 0;
int moistureLevel_4 = 0;

int lightLevel = 0;

// Replace with your network credentials
String wifiSsid;
String wifiPassword;
const char* ssid = "HomeNetwork";
const char* password = "DRXJN525";
const char* apSSID = "ESP32_AccessPoint";
const char* apPassword = "12345678"; // at least 8 chars

AsyncWebServer server(80); // Create an AsyncWebServer object on port 80

bool PUMP_STATE = false; // Ali je pumpa prvotno vklopjena/izklopjena
bool isAuto = true; // default: avtomatski način


void saveSettings() {
  DynamicJsonDocument doc(1024);
  doc["language"] = "en";
  saveJsonToFile("/config.json", doc);
}

unsigned long lastUpdate = 0;
int sensorIntervalMs = 2000; // default 2s

void loadSettings() {
  DynamicJsonDocument doc(1024);
  if (loadJsonFromFile("/config.json", doc)) {
    wifiSsid = doc["wifi_ssid"] | "";
    wifiPassword = doc["wifi_password"] | "";
    sensorIntervalMs = (doc["sensor_update_interval"] | 5) * 1000;
    lcdOn = doc["display_enabled"] | true;

    Serial.printf("Loaded config: SSID=%s, interval=%dms, display=%d\n", 
      ssid.c_str(), sensorIntervalMs, lcdOn);
  }
}

void ensureConfigExists() { // v ensureConfigExists ali default config
  DynamicJsonDocument defaults(512);
  defaults["wifi_ssid"] = "";
  defaults["wifi_password"] = "";
  defaults["sensor_update_interval"] = 2; // default 2 sec
  defaults["display_enabled"] = true;
  ensureJsonFileExists("/config.json", defaults);
}

void saveRules(const String& jsonRules) {
  saveJsonToFile("/rules.json", jsonRules);
}

void loadRules() {
  String json;
  if (loadJsonAsString("/rules.json", json)) {
    Serial.println("Rules: " + json);
  }
}


// Funkcija katera se zažene ob zagonu esp-ja
void setup() {
  Serial.begin(115200); // Inicializacija Serial Monitor (Izpis razhroščevanje podatkov)

  // Nastavitev načina vhoda za pine sensorjev vlaga
  pinMode(MOISTURE_PIN_1, INPUT);
  pinMode(MOISTURE_PIN_2, INPUT);
  pinMode(MOISTURE_PIN_3, INPUT);
  pinMode(MOISTURE_PIN_4, INPUT);

  pinMode(PUMP_PIN, OUTPUT); // Nastavitev vhoda za pumpo
  digitalWrite(PUMP_PIN, PUMP_STATE ? HIGH : LOW); // Prvotno stanje pumpe

  // Inicializacije zaslona
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.setTextSize(2);
  display.print("C.R.K.N.");
  display.setTextSize(1);
  display.setCursor(20, 45);
  display.print("Starting...");
  display.display();
  delay(2000);

  // Inicializaja LittleFS, za datotečni sistem
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("LittleFS Error");
    display.display();
    while(true); // Zamrzni, nekaj je šlo narobe pri inicializaciji
  }

  // Če vklopjeno da se konfiguracija resetira, se zbriše config.json datoteka
  if (LittleFS.exists("/config.json") && reset_settings == true) {
    LittleFS.remove("/config.json");
  }
  ensureConfigExists(); // Preveri ali obstaja konfiguracijska datoteka

  // Inicializacija Wifi/AP
  WiFi.mode(WIFI_AP_STA); // Nastavitev esp-ja da je v Wifi in AP (AccessPoint) načinu
  //WiFi.begin(ssid, password); // Vklopi wifi
  if (wifiSsid != "") {
      WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  } else {
      WiFi.begin(ssid, password); // fallback default, če ni v configu
  }

  // Zagon AccessPoint
  WiFi.softAP(apSSID, apPassword); // SoftAP pomeni, da se ne rabi povezati na WiFi
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP()); // Izpis IP-ja za povezavo na AP

  // Če povezava ne uspe, probaj še 10-krat
  unsigned long startAttemptTime = millis();
  int attemptCounter = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    int dotCount = attemptCounter % 4; // cycles 0-3
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Connecting to WiFi");
    for (int i=0; i<dotCount; i++) {
      display.print(".");
    }
    display.display();
    attemptCounter = attemptCounter + 1;
    delay(500);
  }

  // Če uspešno povezan na wifi, izpiši IP na keterega se povezati
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());
    display.clearDisplay();
    display.setCursor(0,0);
    display.printf("Connected: %s", WiFi.localIP().toString().c_str());
    display.setCursor(0,10);
    display.printf("AP IP: %s", WiFi.softAPIP().toString().c_str());
    display.display();
    delay(2000);
  } else {
    Serial.println("WiFi connection failed, continuing without WiFi");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ // Definicija za root endpoint, prikaže se prvotna stran (index.html)
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Definicija statičnih datotek
  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/script.js", LittleFS, "/script.js");
  server.serveStatic("/automation.js", LittleFS, "/automation.js");
  server.serveStatic("/models/", LittleFS, "/models/"); // zastarelo / ni več v uporabi

  server.on("/signal", HTTP_GET, [](AsyncWebServerRequest *request){ // sprejem signala za preklop pumpe
    PUMP_STATE = !PUMP_STATE;
    digitalWrite(PUMP_PIN, PUMP_STATE ? HIGH : LOW);
    Serial.print("Pin state changed to: ");
    Serial.println(PUMP_STATE ? "OFF" : "ON");
    request->send(200, "text/plain", PUMP_STATE ? "Pin turned ON" : "Pin turned OFF");
  });

  server.on("/set_mode", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    static String body;
    if (index == 0) body = "";
    for (size_t i = 0; i < len; i++) {
      body += (char)data[i];
    }
    if (index + len == total) {
      DynamicJsonDocument doc(256);
      DeserializationError err = deserializeJson(doc, body);
      if (!err && doc.containsKey("auto")) {
        isAuto = doc["auto"];
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"invalid json\"}");
      }
    }
  });

  server.on("/dht_sensor", HTTP_GET, [](AsyncWebServerRequest *request){ // Pošiljanje podatkov za temperaturo in vlago
    String json = "{";
    json += "\"temperature\":" + String(temperature, 1) + ",";
    json += "\"humidity\":" + String(humidity, 1);
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/moisture_sensor", HTTP_GET, [](AsyncWebServerRequest *request){ // Pošiljanje podatkov za vlago tal
    String json = "{";
    json += "\"moisture1\":" + String(moistureLevel_1) + ",";
    json += "\"moisture2\":" + String(moistureLevel_2) + ",";
    json += "\"moisture3\":" + String(moistureLevel_3) + ",";
    json += "\"moisture4\":" + String(moistureLevel_4);
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/light_sensor", HTTP_GET, [](AsyncWebServerRequest *request){ // Pošiljanje podatkov za svetlost
    int lightLevelRaw = analogRead(LIGHT_LEVEL_PIN);
    int lightLux = map(lightLevelRaw, 0, 4095, 1000, 0);
    
    String json = "{";
    json += "\"light\":" + String(lightLux) + "lx";
    json += "}";
    request->send(200, "application/json", json);
  });


  // GET config.json
  server.on("/get_json_data", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json;
    if (loadJsonAsString("/config.json", json)) {
      request->send(200, "application/json", json);
    } else {
      request->send(404, "application/json", "{\"error\":\"File not found or unreadable\"}");
    }
  });

  // POST config.json
  server.on("/save_json_data", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    static String jsonString;
    if (index == 0) jsonString = "";
    for (size_t i = 0; i < len; i++) {
      jsonString += (char)data[i];
    }

    if (index + len == total) {
      DynamicJsonDocument doc(512);
      DeserializationError err = deserializeJson(doc, jsonString);

      if (!err) {
        // če je v JSON-u display_enabled → spremeni lcdOn
        if (doc.containsKey("display_enabled")) {
          lcdOn = doc["display_enabled"];
          if (!lcdOn) {
            display.clearDisplay();
            display.display(); // izklopi takoj
          }
        }

        if (saveJsonToFile("/config.json", jsonString)) {
          request->send(200, "application/json", "{\"status\":\"OK\"}");
        } else {
          request->send(400, "application/json", "{\"status\":\"Failed to save JSON\"}");
        }
      } else {
        request->send(400, "application/json", "{\"status\":\"Invalid JSON\"}");
      }
    }
  });

  server.on("/get_rules", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json;
    if (loadJsonAsString("/rules.json", json)) {
        request->send(200, "application/json", json);
    } else {
        request->send(404, "application/json", "{\"error\":\"rules.json not found or unreadable\"}");
    }
  });

  server.on("/save_rules", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    static String jsonString;  // static so it persists across chunks
    if (index == 0) jsonString = ""; // first chunk, reset
    for (size_t i = 0; i < len; i++) {
      jsonString += (char)data[i];
    }
    if (index + len == total) {
      // last chunk
      if (saveJsonToFile("/rules.json", jsonString)) {
        loadAutomationRules(rules);
        request->send(200, "application/json", "{\"status\":\"OK\"}");
      } else {
        request->send(400, "application/json", "{\"status\":\"Invalid or failed to save JSON\"}");
      }
    }
  });


  // Zagon spletnega strežnika
  server.begin();
  dht.begin(); // Zagon senzorja temperature / vlage

  loadAutomationRules(rules);
}


// Funkcija za konverzijo iz analognega podatka svetlosti v %
int getMoisturePercent(int pin) {
  delay(5);
  int raw = analogRead(pin);
  if (raw < 99) {
    return -1;
  }
  raw = constrain(raw, 100, 3000); 
  int percent = map(raw, 3000, 100, 0, 100);
  return percent;
}


// Funkcija katera vedno dela
void loop() {
  float h = dht.readHumidity(); // branje vlage ozračja
  float t = dht.readTemperature(); // branje temperature

  // Preverjanje celovitosti podatka dht senzorja
  bool dhtOk = (!isnan(h) && !isnan(t));
  if (dhtOk) {
    humidity = h;
    temperature = t;
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }

  // Branje vlage tal
  moistureLevel_1 = getMoisturePercent(MOISTURE_PIN_1);
  moistureLevel_2 = getMoisturePercent(MOISTURE_PIN_2);
  moistureLevel_3 = getMoisturePercent(MOISTURE_PIN_3);
  moistureLevel_4 = getMoisturePercent(MOISTURE_PIN_4);

  // Svetlost
  lightLevel = analogRead(LIGHT_LEVEL_PIN);

  // Posodabljanje zaslona
  if (lcdOn) {
    // Izpis na zaslonu
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    if (dhtOk) {
      display.printf("Temperatura: %.1f C\n", temperature);
      display.printf("Vlaga:  %.1f %%\n", humidity);
    } else {
      display.println("DHT Error");
    }

    display.print("Senzor 1: ");
    display.println(moistureLevel_1 < 0 ? "NC" : String(moistureLevel_1) + "%");

    display.print("Senzor 2: ");
    display.println(moistureLevel_2 < 0 ? "NC" : String(moistureLevel_2) + "%");

    display.print("Senzor 3: ");
    display.println(moistureLevel_3 < 0 ? "NC" : String(moistureLevel_3) + "%");

    display.print("Senzor 4: ");
    display.println(moistureLevel_4 < 0 ? "NC" : String(moistureLevel_4) + "%");

    int lightLevelRaw = analogRead(LIGHT_LEVEL_PIN);
    int lightLux = map(lightLevelRaw, 0, 4095, 1000, 0);
    display.printf("Svetlost: %d lx\n", lightLux);

    display.display();
  }

  int m = getMoisturePercent(MOISTURE_PIN_1); // or any sensor
  int l = analogRead(LIGHT_LEVEL_PIN);

  if (isAuto) {
    evaluateAutomationRules(rules, t, h, m, l, PUMP_PIN);
  } else {
    // manual mode – nič pravil
    // pumpo upravljaš samo preko gumba /signal
  }

  delay(sensorIntervalMs); // posodobi vsaki 2 sekundi
}
