#include <Arduino.h>
#include <permanentDefinitions.h>
#include <SPI.h>
#include <Wire.h>
#include <tinygps++.h>
#include "esp_sleep.h"
#include <elapsedMillis.h>
#include <time.h>
#include <TimeLib.h>
#include "FS.h"
#include "SPIFFS.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// === Declarations ===
TinyGPSPlus gps;
elapsedMillis portableTimer;

RTC_DATA_ATTR uint32_t wakeCounter = 0;
RTC_DATA_ATTR uint32_t wakeTarget = 0; // seconds

RTC_DATA_ATTR int gpsFrequency = 15;   // in minutes
RTC_DATA_ATTR int gpsTimeout = 60;     // in seconds
RTC_DATA_ATTR int gpsHdop = 5;         // HDOP threshold

RTC_DATA_ATTR uint32_t locationCount = 0;

const char* ssid = deviceName; // Use device name as SSID
const char* password = "12345678";

WebServer server(80);

// === Helper Functions ===

String getTagFilename(uint16_t tag) {
  return "/" + String(tag) + ".csv";
}

String formatLocDataAsCSV(const locdata& data) {
  return String(data.count) + "," +
         String(data.datetime) + "," +
         String(data.locktime) + "," +
         String(data.lat, 6) + "," +
         String(data.lng, 6) + "," +
         String(data.hdop);
}

bool mountSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ SPIFFS mount failed!");
    return false;
  }
  Serial.println("✅ SPIFFS mounted.");
  return true;
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

void handleFileDownload() {
  File file = SPIFFS.open(getTagFilename(tag), "r");
  if (!file || file.isDirectory()) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "text/csv");
  file.close();
}

void handleSettingsrequest() {
    char buffer[128];
    JsonDocument doc;
    doc["gpsFrequency"] = gpsFrequency;
    doc["gpsTimeout"] = gpsTimeout; 
    doc["gpsHdop"] = gpsHdop;
    serializeJson(doc, buffer);
    server.send(200, "application/json", buffer);
    Serial.println("📄 Settings sent to client.");
    Serial.flush();
}

void handleSettingsPost() {
  if (server.hasArg("plain") == false) {
    server.send(400, "text/plain", "Bad Request: No body");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    server.send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
    return;
  }

  // Optional: validate types
  if (doc.containsKey("gfrq")) gpsFrequency = doc["gfrq"];
  if (doc.containsKey("gto"))  gpsTimeout   = doc["gto"];
  if (doc.containsKey("hdop")) gpsHdop      = doc["hdop"];

  // Optional: store to NVS or SPIFFS for persistence

  char response[128];
  snprintf(response, sizeof(response),
           "{\"gfrq\": %d, \"gto\": %.2f, \"hdop\": %.2f}",
           gpsFrequency, (float)gpsTimeout, (float)gpsHdop);

  server.send(200, "application/json", response);
}

void runHttpServerWithTimeout(unsigned long timeoutMillis = 300000) {
  WiFi.mode(WIFI_AP);
  WiFi.setTxPower(WIFI_POWER_15dBm);
  WiFi.softAP(ssid, password);
  Serial.print("📡 AP started at: ");
  Serial.println(WiFi.softAPIP());

  server.on("/gps.csv", HTTP_GET, handleFileDownload);
  server.on("/settings", HTTP_GET, handleSettingsrequest);
  server.on("/settings", HTTP_POST, handleSettingsPost);
  server.begin();
  Serial.println("✅ HTTP server started");

  unsigned long startTime = millis();

  // Keep running until timeout
  while (millis() - startTime < timeoutMillis) {
    server.handleClient();
    delay(10);
  }

  Serial.println("⏹️ HTTP server timeout reached. Shutting down.");

  // Stop Wi-Fi and BT to save power
  server.stop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
}

bool shouldRunTask() {
  wakeCounter++;
  Serial.printf("Wake count: %lu\n", wakeCounter);

  if (wakeCounter >= wakeTarget) {
    wakeCounter = 0;
    return true;
  }

  esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_SECONDS * 1000000ULL);
  esp_deep_sleep_start();
  return false;
}

void recGPS() {
  double hdopstrt = 10.00;
  portableTimer = 0;
  double lat, lng;

  digitalWrite(GPS_EN, HIGH);

  while ((portableTimer / 1000) < gpsTimeout) {
    while (Serial1.available() > 0) {
      if (!gps.encode(Serial1.read())) {
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
}

void setup() {

    Serial.begin(115200);
    delay(500);

    mountSPIFFS();

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    esp_reset_reason_t resetReason = esp_reset_reason();

    Serial.printf("Wakeup cause: %d, Reset reason: %d\n", cause, resetReason);

    if (cause == ESP_SLEEP_WAKEUP_UNDEFINED && resetReason == ESP_RST_POWERON) {
      Serial.println("⚡ Power-on detected: Entering server mode");
      Serial.println(ESP.getFreeHeap());
      Serial.flush();
      runHttpServerWithTimeout(300000); // 5 minutes
    }

    if (!shouldRunTask()) {
      return;
    }

    Serial.println("⏰ Running scheduled task...");
    recGPS();

    wakeTarget = (gpsFrequency * 60) - (portableTimer / 1000);
    Serial.printf("⏱️ Sleeping for next %lus\n", wakeTarget);
    Serial.flush(); 

    // Turn off Wi-Fi and Bluetooth before sleeping
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
    esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_SECONDS * 1000000ULL);
    esp_deep_sleep_start();

}

void loop() {
  // not used
}
