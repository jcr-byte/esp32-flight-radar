#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"
#include "flight_tracker.h"
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

// Joystick edge states
int prevYAxisState = 0;
int prevXAxisState = 0;
int prevButtonState = 1; 

// Screen states
enum class Screen { Radar, Details };
Screen currentScreen = Screen::Radar;

FlightTracker tracker(currentLat, currentLon);

void drawRadar() {
  ui_draw_flights(tracker.flights(), currentLat, currentLon,
                  tracker.selectedIndex(), tracker.stale());
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

  tracker.refresh();
  lastUpdate = millis();
  Serial.printf("Got %u flights; stale=%d\n", tracker.flights().size(), tracker.stale());
  drawRadar();

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
    tracker.moveSelection(dirX, 0);
    drawRadar();
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
    tracker.moveSelection(0, dirY);
    drawRadar();
  }

  int buttonState = digitalRead(JOY_SW_PIN);
  if (buttonState == 0 && prevButtonState == 1) {
    if (const Flight* selected = tracker.selectedFlight()) {
      Serial.print("CONFIRMED flight ");
      Serial.println(selected->icao24);
      ui_draw_details_page(*selected);
      currentScreen = Screen::Details;
    }
  }

  if (currentScreen == Screen::Radar && millis() - lastUpdate >= UPDATE_INTERVAL_MS) {
    lastUpdate = millis();

    tracker.refresh();
    drawRadar();
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
    drawRadar();
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

