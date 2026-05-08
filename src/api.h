#pragma once

#include <Arduino.h>

struct Flight {
    String icao24;
    String callsign;
    float lat;
    float lon;
    float altitude;
    float velocity;
    float heading;
    bool onGround;
};

String fetchToken();
String getAccessToken();
std::vector<Flight> getFlights(const double currentLat, const double currentLong);