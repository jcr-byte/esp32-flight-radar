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
int selectedIndex = -1;
int confirmedIndex = -1;
int prevYAxisState = 0;
int prevXAxisState = 0;
int prevButtonState = 1; 

// Screen states
enum class Screen { Radar, Details };
Screen currentScreen = Screen::Radar;

// Flights array
std::vector<Flight> flights;
bool flightDataStale = true;

void refreshFlights() {
  FlightFetchResult result = getFlights(currentLat, currentLon);
  if (!result.success) {
    flightDataStale = true;
    return;
  }

  String selectedId;
  if (selectedIndex >= 0 && selectedIndex < int(flights.size())) {
    selectedId = flights[selectedIndex].icao24;
  }

  flights.swap(result.flights);
  flightDataStale = false;
  selectedIndex = -1;
  for (size_t i = 0; i < flights.size(); ++i) {
    if (!calculate_flight_positions(flights[i], currentLat, currentLon).inRange) continue;
    if (selectedIndex == -1) selectedIndex = int(i);
    if (!selectedId.isEmpty() && flights[i].icao24 == selectedId) {
      selectedIndex = int(i);
      break;
    }
  }
}

// put function declarations here:
int findNeighbor(int dirX, int dirY, int fromIndex, const std::vector<Flight>& flights) {
  if (fromIndex < 0 || fromIndex >= int(flights.size())) return -1;
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

  refreshFlights();
  lastUpdate = millis();
  Serial.printf("Got %u flights; stale=%d\n", flights.size(), flightDataStale);
  ui_draw_flights(flights, currentLat, currentLon, selectedIndex, flightDataStale);

}

void handleRadarSreen() {
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
      ui_draw_flights(flights, currentLat, currentLon, selectedIndex, flightDataStale);
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
      ui_draw_flights(flights, currentLat, currentLon, selectedIndex, flightDataStale);
    }
  }

  int buttonState = digitalRead(JOY_SW_PIN);
  if (buttonState == 0 && prevButtonState == 1 && selectedIndex >= 0 && selectedIndex < int(flights.size())) {
    confirmedIndex = selectedIndex;
    Serial.print("CONFIRMED flight");
    Serial.println(confirmedIndex);

    ui_clear_screen();
    ui_draw_details_page(flights[selectedIndex]);
    currentScreen = Screen::Details;
  }

  if (currentScreen == Screen::Radar && millis() - lastUpdate >= UPDATE_INTERVAL_MS) {
    lastUpdate = millis();

    refreshFlights();
    ui_draw_flights(flights, currentLat, currentLon, selectedIndex, flightDataStale);
  }

  prevYAxisState = yAxisState;
  prevXAxisState = xAxisState;
  prevButtonState = buttonState;
  delay(20);
}

void handleDetailsScreen() {
  

  int buttonState = digitalRead(JOY_SW_PIN);
  if (buttonState == 0 && prevButtonState == 1) {

    ui_clear_screen();
    ui_draw_flights(flights, currentLat, currentLon, selectedIndex, flightDataStale);
    currentScreen = Screen::Radar;
  }

  prevButtonState = buttonState;
  delay(20);
}

void loop() {
  switch (currentScreen) {
    case Screen::Radar: handleRadarSreen(); break;
    case Screen::Details: handleDetailsScreen(); break;
  }
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}

