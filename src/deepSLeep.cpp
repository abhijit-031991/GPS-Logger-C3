#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <esp_sleep.h>
#include <permanentDefinitions.h>
#include <ArduinoJson.h>
#include <elapsedMillis.h>
#include <TinyGPS++.h>
#include <time.h>
#include <TimeLib.h>
#include "esp_wifi.h"
#include "esp_netif.h"


AsyncWebServer server(80); 
elapsedMillis portableTimer;
TinyGPSPlus gps;
testGPS gpsData;

const char* ssid = "ArcTrack-Passkey";
const char* password = "password";  // Minimum 8 characters

RTC_DATA_ATTR uint32_t wakeCounter = 0;
RTC_DATA_ATTR uint32_t wakeTarget = 1; // seconds

RTC_DATA_ATTR int gpsFrequency = 3;   // in minutes
RTC_DATA_ATTR int gpsTimeout = 120;     // in seconds
RTC_DATA_ATTR int gpsHdop = 5;         // HDOP threshold


RTC_DATA_ATTR uint32_t locationCount = 0;

RTC_DATA_ATTR bool isFirstRun = true; // Track if this is the first run after boot
RTC_DATA_ATTR bool userInit = false;

testGPS testGps(){ 
    testGPS temp;
    Serial0.begin(115200); // RX, TX pins for GPS
    delay(1000); // Allow time for GPS to initialize
    digitalWrite(GPS_EN, HIGH);
    Serial.println(F("GPS Starting"));
    do{ 
        Serial.flush();
        while (Serial0.available() > 0)
        {
            if (gps.encode(Serial0.read()))
            {
                if (!gps.location.isValid())
                {
                Serial.println(F("No GPS Fix"));
                }else{                      
                Serial.print(F("Satellites:"));
                Serial.println(gps.satellites.value());
                Serial.print(F("HDOP:"));
                Serial.println(gps.hdop.hdop());
                if (gps.hdop.hdop() != 0.00  && gps.hdop.hdop() < 5.00)
                    {
                        Serial.println(gps.hdop.hdop());
                    }
                }
            }
        }
    }while(!gps.location.isValid() && gps.location.age() > 100); 
    temp.gps = true; 
    temp.lat = gps.location.lat();
    temp.lng = gps.location.lng();
    Serial.flush();  
    digitalWrite(GPS_EN, LOW);
    Serial0.end(); // Stop GPS Serial
    return temp;
}

String formatLocDataAsCSV(const locdata& data) {
  return String(data.count) + "," +
         String(data.datetime) + "," +
         String(data.locktime) + "," +
         String(data.lat, 6) + "," +
         String(data.lng, 6) + "," +
         String(data.hdop);
}

float readBatteryVoltage() {
  // Read raw ADC value
  uint16_t raw = analogRead(VSNS_PIN);

  // Get Vref (ESP32-C3 has calibrated value stored in eFuse)
  uint32_t vref = VREF; // Or use adcReadVref() if available with ADC calibration

  // Convert ADC value to input voltage (in millivolts)
  float vInput_mV = ((float)raw / ADC_MAX_VALUE) * vref;

  // Since voltage divider halves the battery voltage:
  float batteryVoltage = vInput_mV * 2.0 / 1000.0; // Convert to volts

  return batteryVoltage;
}

bool appendToCSV(const String& path, const String& line) {
  File file = SPIFFS.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("❌ Failed to open file for appending.");
    return false;
  }
  file.println(line);
  file.close();
  Serial.println("✅ Data appended to file.");
  return true;
}

void createCSVWithHeader(const char* path, const char* header) {
  if (!SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, FILE_WRITE);
    if (file) {
      file.println(header);
      file.close();
      Serial.println("📄 Header written to new CSV.");
    }
  }
}

String getTagFilename(uint16_t tag) {
  return "/" + String(tag) + ".csv";
}

void handleFileDownload(AsyncWebServerRequest *request) {// Async handler for file download
  String filename = getTagFilename(tag);
  
  if (!SPIFFS.exists(filename)) {
    request->send(404, "text/plain", "File not found");
    return;
  }
  
  // Send file with proper headers
  request->send(SPIFFS, filename, "text/csv", true);
  Serial.println("📄 File download requested");
}

void handleSettingsRequest(AsyncWebServerRequest *request) {// Async handler for settings GET request
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  
  JsonDocument doc;
  doc["gpsFrequency"] = gpsFrequency;
  doc["gpsTimeout"] = gpsTimeout; 
  doc["gpsHdop"] = gpsHdop;
  doc["batteryVoltage"] = readBatteryVoltage();
  doc["gpsCalibrated"] = gpsData.gps;
  doc["lat"] = gpsData.lat;
  doc["lng"] = gpsData.lng;
  doc["id"] = tag;
  
  serializeJson(doc, *response);
  request->send(response);
  
  Serial.println("📄 Settings sent to client.");
}

void handleSettingsPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {// Async handler for settings POST request
  // This is called when body data is received
  if (index == 0) {
    // First chunk of data
    String body = String((char*)data).substring(0, len);
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);

    if (err) {
      request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
      return;
    }

    // Update settings
    if (doc.containsKey("gfrq")) gpsFrequency = doc["gfrq"];
    if (doc.containsKey("gto"))  gpsTimeout   = doc["gto"];
    if (doc.containsKey("hdop")) gpsHdop      = doc["hdop"];
    if (doc.containsKey("userInit")) userInit = doc["userInit"];    

    // Send response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    
    JsonDocument responseDoc;
    responseDoc["gfrq"] = gpsFrequency;
    responseDoc["gto"] = gpsTimeout;
    responseDoc["hdop"] = gpsHdop;
    
    serializeJson(responseDoc, *response);
    request->send(response);
    
    Serial.println("⚙️ Settings updated");
  }
}

void recGPS() {
    double hdopstrt = 10.00;
    portableTimer = 0;
    double lat, lng;

    Serial0.begin(115200); // RX, TX pins for GPS

    Serial.println("🌍 Starting GPS recording...");

    digitalWrite(GPS_EN, HIGH);

    while ((portableTimer / 1000) < gpsTimeout) {
        while (Serial0.available() > 0) {
        if (!gps.encode(Serial0.read())) {
            if (!gps.location.isValid()) {
            Serial.println(F("Acquiring..."));
            }
        } else {
            Serial.print(F("HDOP: "));
            Serial.println(gps.hdop.value());
            if (gps.hdop.hdop() != 0.00) {
            hdopstrt = gps.hdop.hdop();
            }
        }
        }

        if (hdopstrt < (double)gpsHdop &&
            gps.location.age() < 1000 &&
            gps.time.age() < 1000 &&
            gps.hdop.age() < 50) {
        break;
        }
    }

    digitalWrite(GPS_EN, LOW);

    locdata dat;

    if (gps.location.age() < 60000) {
        dat.lat = gps.location.lat();
        dat.lng = gps.location.lng();
    } else {
        dat.lat = 0;
        dat.lng = 0;
    }

    setTime(gps.time.hour(), gps.time.minute(), gps.time.second(),
            gps.date.day(), gps.date.month(), gps.date.year());

    dat.datetime = now();
    dat.locktime = (portableTimer / 1000);
    dat.hdop = gps.hdop.hdop();
    dat.count = ++locationCount;

    Serial.println("🌍 Data:");
    Serial.println(dat.datetime);
    Serial.println(dat.lat);
    Serial.println(dat.lng);
    Serial.println(dat.locktime);
    Serial.println(dat.hdop);
    Serial.println(dat.count);

    String filename = getTagFilename(tag);
    createCSVWithHeader(filename.c_str(), "count,datetime,locktime,lat,lng,hdop");

    String row = formatLocDataAsCSV(dat);
    appendToCSV(filename, row);

    Serial0.end(); // Stop GPS serial communication
}

void httpServerInit(){
    WiFi.mode(WIFI_OFF);

    gpsData = testGps();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    delay(100);

    Serial.println("📶 Connecting to WiFi...");
    WiFi.begin(ssid, password);
    esp_wifi_set_max_tx_power(52);

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n❌ Failed to connect to WiFi.");
        return;
    }

    Serial.println("\n✅ WiFi connected!");
    Serial.print("📡 Local IP: ");
    Serial.println(WiFi.localIP());

    // Setup async routes
    server.on("/gps.csv", HTTP_GET, handleFileDownload);
    server.on("/settings", HTTP_GET, handleSettingsRequest);

    server.on("/settings", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            // Empty handler for body processing
        }, 
        NULL, 
        handleSettingsPost
    );

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    server.begin();
    Serial.println("✅ Async HTTP server started");

    
    portableTimer = 0; // Reset the timer

    while (portableTimer < WIFI_TIMEOUT_MS || gpsData.gps == true) {
        if (userInit) {
            Serial.println("✅ User initialization complete, exiting WiFi setup loop.");
            break;
        }                         
    }
}

void setup() {
    delay(10000); // brief delay for serial if debugging
    Serial.begin(115200);
    Serial.println("Starting setup...");

    pinMode(GPS_EN, OUTPUT);
    digitalWrite(GPS_EN, LOW); // Disable GPS initially

    if (isFirstRun) {
        httpServerInit(); // Initialize HTTP server only on first run
        isFirstRun = false; // only first boot
    }else{
        bool x = testGps().gps;
    }

    

    if (!SPIFFS.begin(true)) {
        Serial.println("❌ SPIFFS mount failed!");
    }
    Serial.println("✅ SPIFFS mounted.");

    Serial.println(isFirstRun ? "First run detected, HTTP server initialized." : "Not first run, skipping HTTP server initialization.");

    

    // Clean up
    SPIFFS.end();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    Serial.flush();
    delay(100);

    // Set sleep timer and enter deep sleep
    esp_sleep_enable_timer_wakeup(180 * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() {
  // never runs
}
