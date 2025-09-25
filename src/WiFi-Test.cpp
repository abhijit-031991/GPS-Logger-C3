// #include <WiFi.h>
// #include <WebServer.h>
// #include "esp_wifi.h"
// #include "esp_netif.h"

// const char* ssid     = "ArcTrack-Passkey";
// const char* password = "password";

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   Serial.println();
//   Serial.println("Simple WiFi Connect");

//   WiFi.mode(WIFI_STA);         // station mode
//   WiFi.disconnect(true);       // clear previous config
//   delay(100);

//   Serial.printf("Connecting to %s\n", ssid);
//   WiFi.begin(ssid, password);
//   esp_wifi_set_max_tx_power(34);

//   unsigned long start = millis();
//   while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
//     Serial.print(".");
//     delay(500);
//   }

//   if (WiFi.status() == WL_CONNECTED) {
//     Serial.println("\n✅ Connected!");
//     Serial.print("IP: ");
//     Serial.println(WiFi.localIP());
//     Serial.print("RSSI: ");
//     Serial.print(WiFi.RSSI());
//     Serial.println(" dBm");
//   } else {
//     Serial.println("\n❌ Failed to connect (timeout)");
//   }
// }

// void loop() {
//   // Basic watchdog-friendly reconnect logic
//   static unsigned long lastCheck = 0;
//   if (millis() - lastCheck > 5000) {
//     lastCheck = millis();
//     if (WiFi.status() != WL_CONNECTED) {
//       Serial.println("WiFi lost. Reconnecting...");
//       WiFi.disconnect(true);
//       WiFi.begin(ssid, password);
//     } else {
//       Serial.print("Alive. IP: ");
//       Serial.print(WiFi.localIP());
//       Serial.print(" RSSI: ");
//       Serial.print(WiFi.RSSI());
//       Serial.println(" dBm");
//     }
//   }
// }
