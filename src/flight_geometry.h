#pragma once
#include "api.h"

namespace RadarGeometry {
constexpr int CENTER_X = 64;
constexpr int CENTER_Y = 60; // sprite starts at screen y=16
constexpr int RADIUS = 50;
constexpr double MAX_RANGE = 100000.0;
}

struct FlightPosition {
    int x;
    int y;
    bool inRange;
};

FlightPosition calculate_flight_positions(const Flight& flight, double latitude, double longitude);
int calculateRadiusDistances(int radius);
