#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <vector>
#include "secrets.h"
#include "api.h"
#include "ui.h"

// Update states
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL_MS = 10000;

// Location states
double currentLat = 44.86499233980789;
double currentLon = -123.34256875649267;

// Joystick Pins
const int JOY_VRX_PIN = 34;
const int JOY_VRY_PIN = 35;
const int JOY_SW_PIN = 32;

// Joystick constants
const int JOY_Y_CENTER = 1880;
const int JOY_X_CENTER = 1935;
const int JOY_DEAD_ZONE = 800;

// Plane selection states
int selectedIndex = 0;
int confirmedIndex = -1;
int prevYAxisState = 0;
int prevXAxisState = 0;
int prevButtonState = 1; 

// Flights array
std::vector<Flight> flights;

// put function declarations here:
int findNeighbor(int dirX, int dirY, int fromIndex, const std::vector<Flight>& flights) {
  FlightPosition currentFlightPos = calculate_flight_positions(flights[fromIndex], currentLat, currentLon);
  int bestIndex = -1;
  int bestScore = 100000;

  for (int i = 0; i < flights.size(); ++i) {
    if (i == fromIndex) continue;

    FlightPosition loopFlightPos = calculate_flight_positions(flights[i], currentLat, currentLon);
    if (!loopFlightPos.inRange) continue;

    int dx = loopFlightPos.x - currentFlightPos.x;
    int dy = loopFlightPos.y - currentFlightPos.y;

    int along = dx * dirX + dy * dirY;
    if (along <= 0) continue;

    int perpendicular = abs(dx * dirY - dy * dirX);

    int score = along + perpendicular * 2;
    if (score < bestScore) {
      bestScore = score;
      bestIndex = i;
    }
  }

  return bestIndex;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  ui_init();

  pinMode(JOY_SW_PIN, INPUT_PULLUP);

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

  flights = getFlights(currentLat, currentLon);
  Serial.printf("Got %u flights\n", flights.size());

  ui_draw_flights(flights, currentLat, currentLon, selectedIndex);


}

void loop() {
  int yAxisValue = analogRead(JOY_VRY_PIN);
  int yAxisState = 0;
  if (yAxisValue < JOY_Y_CENTER - JOY_DEAD_ZONE) {
    yAxisState = -1;
  } else if (yAxisValue > JOY_Y_CENTER + JOY_DEAD_ZONE) {
    yAxisState = 1;
  }

  if (yAxisState != prevYAxisState && yAxisState != 0) {
    int dirX = (yAxisState == -1) ? 1 : -1;
    int next = findNeighbor(dirX, 0, selectedIndex, flights);
    if (next != -1) {
      selectedIndex = next;
      ui_draw_flights(flights, currentLat, currentLon, selectedIndex);
    }
  }

  int xAxisValue = analogRead(JOY_VRX_PIN);
  int xAxisState = 0;
  if (xAxisValue < JOY_X_CENTER - JOY_DEAD_ZONE) {
    xAxisState = -1;
  } else if (xAxisValue > JOY_X_CENTER + JOY_DEAD_ZONE) {
    xAxisState = 1;
  }

  if (xAxisState != prevXAxisState && xAxisState != 0) {
    int dirY = (xAxisState == -1) ? -1 : 1;
    int next = findNeighbor(0, dirY, selectedIndex, flights);
    if (next != -1) {
      selectedIndex = next;
      ui_draw_flights(flights, currentLat, currentLon, selectedIndex);
    }
  }

  int buttonState = digitalRead(JOY_SW_PIN);
  if (buttonState == 0 && prevButtonState == 1) {
    confirmedIndex = selectedIndex;
    Serial.print("CONFIRMED flight");
    Serial.println(confirmedIndex);
  }

  if (millis() - lastUpdate >= UPDATE_INTERVAL_MS) {
    lastUpdate = millis();

    flights = getFlights(currentLat, currentLon);
    ui_draw_flights(flights, currentLat, currentLon, selectedIndex);
  }

  prevYAxisState = yAxisState;
  prevXAxisState = xAxisState;
  prevButtonState = buttonState;
  delay(20);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}

