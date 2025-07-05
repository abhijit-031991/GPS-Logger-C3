// #include <WiFi.h>

// const wifi_power_t powerLevels[] = {
//   WIFI_POWER_2dBm,
//   WIFI_POWER_5dBm,
//   WIFI_POWER_7dBm,
//   WIFI_POWER_11dBm,
//   WIFI_POWER_13dBm,
//   WIFI_POWER_15dBm,
//   WIFI_POWER_17dBm,
//   WIFI_POWER_19_5dBm
// };

// const char* powerLabels[] = {
//   "2dBm", "5dBm", "7dBm", "11dBm", "13dBm", "15dBm", "17dBm", "19.5dBm"
// };

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   for (int i = 0; i < sizeof(powerLevels) / sizeof(powerLevels[0]); i++) {
//     Serial.printf("\n🚦 Testing TX power: %s\n", powerLabels[i]);
//     WiFi.mode(WIFI_OFF);
//     delay(100);
//     WiFi.mode(WIFI_AP);
//     WiFi.setTxPower(powerLevels[i]);
//     WiFi.softAP("C3Test", "12345678");
//     Serial.println("✅ AP started at: " + WiFi.softAPIP().toString());
//     delay(5000);  // Hold for 5 seconds

//     // Optionally: blink LED, measure heap, do a ping, etc.
//   }

//   Serial.println("\n✅ Completed all power levels.");
// }

// void loop() {}
