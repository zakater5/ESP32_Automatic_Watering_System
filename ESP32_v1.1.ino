#include <LittleFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ArduinoJson.h>

bool reset_settings = false; // Če vklopjeno se resetira vsa konfiguracija nazaj na prvotno ob zagonu

// display:
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Reset pin isn't used
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// button:
bool lcdOn = true;

#define DHTPIN 23       // Change to the pin you're using
#define DHTTYPE DHT11   // Or DHT11
//#define DHTTYPE DHT21    // Use DHT21 for AM2301A

DHT dht(DHTPIN, DHTTYPE);
float temperature = 0.0; // spremenljivka v katero se piše temperatura
float humidity = 0.0; // spremenljivka v katero se piše vlaga ozračja

#define MOISTURE_PIN_1 33  // ADC pin for analog output
int moistureLevel_1 = 0;
#define MOISTURE_PIN_2 32  // ADC pin for analog output
int moistureLevel_2 = 0;
#define MOISTURE_PIN_3 35  // ADC pin for analog output
int moistureLevel_3 = 0;
#define MOISTURE_PIN_4 34  // ADC pin for analog output
int moistureLevel_4 = 0;

const int LIGHT_LEVEL_PIN = 36;
int lightLevel = 0;

// Replace with your network credentials
const char* ssid = "HomeNetwork";
const char* password = "DRXJN525";
const char* apSSID = "ESP32_AccessPoint";
const char* apPassword = "12345678"; // at least 8 chars

const int pin = 13;  // Pin kateri vklaplja pumpo

AsyncWebServer server(80); // Create an AsyncWebServer object on port 80

bool pinState = false; // Ali je pumpa prvotno vklopjena/izklopjena

// Funkcija za shranjevanje konfiguracije v config.json datoteko
void saveSettings() {
  StaticJsonDocument<256> doc;
  doc["language"] = "en";

  File file = LittleFS.open("/config.json", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write JSON to file");
  } else {
    Serial.println("JSON saved successfully");
  }
  file.close();
}

// Funkcija za nalaganje konfiguracije / branje iz config.json datoteke
void loadSettings() {
  File file = LittleFS.open("config.json", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print("Failed to read JSON: ");
    Serial.println(error.c_str());
    return;
  }

  String language = doc["language"];
  Serial.println("Language: " + language);
  file.close();
}

// Funkcija katera preveri ali datoteka config.json obstaja, če ne, jo naredi
void ensureJsonFileExists() {
  if (!LittleFS.exists("/config.json")) {
    Serial.println("config.json not found, creating default one...");
    File file = LittleFS.open("/config.json", "w");
    if (!file) {
      Serial.println("Failed to create config.json");
      return;
    }

    StaticJsonDocument<256> doc;
    doc["language"] = "en";

    if (serializeJson(doc, file) == 0) {
      Serial.println("Filed to write default JSON");
    } else {
      Serial.println("Default config.json created.");
    }
    file.close();
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

  pinMode(pin, OUTPUT); // Nastavitev vhoda za pumpo
  digitalWrite(pin, pinState ? HIGH : LOW); // Prvotno stanje pumpe

  // Inicializaja LittleFS, za datotečni sistem
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    while(true); // Zamrzni, nekaj je šlo narobe pri inicializaciji
  }

  // Če vklopjeno da se konfiguracija resetira, se zbriše config.json datoteka
  if (LittleFS.exists("/config.json") && reset_settings == true) {
    LittleFS.remove("/config.json");
  }
  ensureJsonFileExists(); // Preveri ali obstaja konfiguracijska datoteka

  // Inicializacija Wifi/AP
  WiFi.mode(WIFI_AP_STA); // Nastavitev esp-ja da je v Wifi in AP (AccessPoint) načinu
  WiFi.begin(ssid, password); // Vklopi wifi

  // Če povezava ne uspe, probaj še 10-krat
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Če uspešno povezan na wifi, izpiši IP na keterega se povezati
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed, continuing without WiFi");
  }

  // Zagon AccessPoint
  WiFi.softAP(apSSID, apPassword); // SoftAP pomeni, da se ne rabi povezati na WiFi
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP()); // Izpis IP-ja za povezavo na AP

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ // Definicija za root endpoint, prikaže se prvotna stran (index.html)
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Definicija statičnih datotek
  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/script.js", LittleFS, "/script.js");
  server.serveStatic("/models/", LittleFS, "/models/"); // zastarelo / ni več v uporabi

  server.on("/signal", HTTP_GET, [](AsyncWebServerRequest *request){ // sprejem signala za preklop pumpe
    pinState = !pinState;
    digitalWrite(pin, pinState ? HIGH : LOW);
    Serial.print("Pin state changed to: ");
    Serial.println(pinState ? "OFF" : "ON");
    request->send(200, "text/plain", pinState ? "Pin turned ON" : "Pin turned OFF");
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

  server.on("/get_json_data", HTTP_GET, [](AsyncWebServerRequest *request) { // Pošiljanje konfiguracije
    if (LittleFS.exists("/config.json")) {
      File file = LittleFS.open("/config.json", "r");
      if (!file) {
        request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
        return;
      }
      String json = file.readString();
      file.close();

      request->send(200, "application/json", json);
    } else {
      request->send(404, "application/json", "{\"error\":\"File not found\"}");
    }
  });

  // Endpoint za shranjevanje konfiguracije
  server.on("/save_json_data", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String jsonString;
    for (size_t i = 0; i < len; i++) {
      jsonString += (char)data[i];
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
      request->send(400, "application/json", "{\"status\":\"Invalid JSON\"}");
      return;
    }

    File file = LittleFS.open("/config.json", "w");
    if (!file) {
      request->send(500, "application/json", "{\"status\":\"Failed to open file\"}");
      return;
    }

    serializeJson(doc, file);
    file.close();

    request->send(200, "application/json", "{\"status\":\"OK\"}");
  });

  // Zagon spletnega strežnika
  server.begin();
  dht.begin(); // Zagon senzorja temperature / vlage

  // Inicializacije zaslona
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Hello ESP32!"); // za testiranje
  display.display();
}


// Funkcija za konverzijo iz analognega podatka svetlosti v %
int getMoisturePercent(int pin) {
  int raw = analogRead(pin);
  if (raw < 100) {
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

  delay(2000); // posodobi vsaki 2 sekundi
}
