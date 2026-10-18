#pragma once

#include <vector>
#include <Arduino.h>

struct Flight {
    String icao24;
    String callsign;
    String originCountry;
    float lat;
    float lon;
    float altitude;
    float velocity;
    float heading;
    bool onGround;
};

struct FlightFetchResult {
    bool success;
    std::vector<Flight> flights;
};

String fetchToken();
String getAccessToken();
FlightFetchResult getFlights(const double currentLat, const double currentLong);
