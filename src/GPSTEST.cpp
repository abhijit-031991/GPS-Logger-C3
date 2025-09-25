// #include <Arduino.h>
// #include <WiFi.h>
// #include <ESPAsyncWebServer.h>
// #include <SPIFFS.h>
// #include <esp_sleep.h>
// #include <permanentDefinitions.h>
// #include <ArduinoJson.h>
// #include <elapsedMillis.h>
// #include <TinyGPS++.h>
// #include <time.h>
// #include <TimeLib.h>
// #include <HardwareSerial.h>



// void setup() {

//     delay(10000); // brief delay for serial if debugging
//     Serial.begin(115200);
//     Serial.println("GPS Test Setup");

//     // Initialize GPS module
//     Serial0.begin(115200); // RX, TX pins for GPS
//     pinMode(GPS_EN, OUTPUT);
//     digitalWrite(GPS_EN, HIGH); // Enable GPS

//     delay(2000); // Allow GPS to initialize
// }

// void loop() {
//     if (Serial0.available()) {
//         Serial.write(Serial0.read()); // Echo GPS data to Serial Monitor
//     } // Delay for readability
//     if (Serial.available()) {
//         Serial0.write(Serial.read()); // Echo GPS data to Serial Monitor
//     } 
// }