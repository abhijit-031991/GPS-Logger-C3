// #include <WiFi.h>
// #include <WiFiClient.h>
// #include <HTTPClient.h>
// #include <WebServer.h>
// #include <ArduinoJson.h>
// #include "esp_wifi.h"
// #include "esp_netif.h"

// // Create a web server on port 80
// WebServer server(80);

// // Global JSON document to hold the mapping
// StaticJsonDocument<1024> summaryJson;

// void scanAndQueryDevices() {
//   wifi_sta_list_t sta_list;
//   tcpip_adapter_sta_list_t adapter_sta_list;

//   // Get the list of connected stations
//   if (esp_wifi_ap_get_sta_list(&sta_list) != ESP_OK) {
//     Serial.println("Failed to get station list");
//     return;
//   }

//   if (tcpip_adapter_get_sta_list(&sta_list, &adapter_sta_list) != ESP_OK) {
//     Serial.println("Failed to get adapter station list");
//     return;
//   }

//   Serial.printf("Found %d connected device(s)\n", adapter_sta_list.num);

//   // Clear previous data
//   summaryJson.clear();

//   // Iterate through connected stations
//   for (int i = 0; i < adapter_sta_list.num; ++i) {
//     tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
//     IPAddress clientIP(station.ip.addr);
//     String url = "http://" + clientIP.toString() + "/settings";

//     HTTPClient http;
//     http.begin(url);
//     http.setTimeout(2000); // 2 seconds timeout

//     int httpCode = http.GET();

//     if (httpCode == 200) {
//       String payload = http.getString();

//       StaticJsonDocument<256> doc;
//       DeserializationError error = deserializeJson(doc, payload);
//       if (!error && doc.containsKey("id")) {
//         String id = doc["id"].as<String>();
//         summaryJson[id] = clientIP.toString();
//         Serial.printf("Mapped device ID %s to IP %s\n", id.c_str(), clientIP.toString().c_str());
//       } else {
//         Serial.println("JSON parsing error or 'id' not found in response");
//       }
//     } else {
//       Serial.printf("HTTP GET failed for %s with code %d\n", clientIP.toString().c_str(), httpCode);
//     }

//     http.end();
//   }

//   Serial.println("Updated device summary:");
//   serializeJsonPretty(summaryJson, Serial);
// }

// void setup() {
//   Serial.begin(115200);
//   delay(5000);
//   Serial.println("Starting Passkey Device...");
  
// //   WiFi.setTxPower(WIFI_POWER_15dBm); // Set transmit power
//   // Start SoftAP
//   WiFi.softAP("ArcTrack-Passkeyy", "password", 1, false, 20);
//   Serial.println("SoftAP started. IP: " + WiFi.softAPIP().toString());
//   esp_wifi_set_max_tx_power(52); 

//   // Setup web server route
//   server.on("/summary", HTTP_GET, []() {
//     String response;
//     serializeJson(summaryJson, response);
//     server.send(200, "application/json", response);
//   });

//   server.begin();
//   Serial.println("HTTP server started.");
// }

// void loop() {
//   server.handleClient();

//   // Every 10 seconds, refresh the summary
//   static unsigned long lastScan = 0;
//   if (millis() - lastScan > 10000) {
//     lastScan = millis();
//     scanAndQueryDevices();
//   }
// }