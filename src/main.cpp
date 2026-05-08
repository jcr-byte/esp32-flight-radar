#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "api.h"
#include "ui.h"


// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  ui_init();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.println(".");
    if (++attempts > 60) {
      Serial.println("\nWiFi failed");
      return;
    }
  }
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  String rawToken = getAccessToken();
  
  Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
  Serial.printf("Largest block: %u\n", ESP.getMaxAllocHeap());
  Serial.printf("Flash size: %u\n", ESP.getFlashChipSize());

  double currentLat = 44.86499233980789;
  double currentLon = -123.34256875649267;
  std::vector<Flight> flights = getFlights(currentLat, currentLon);
  Serial.printf("Got %u flights\n", flights.size());

}

void loop() {

}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}