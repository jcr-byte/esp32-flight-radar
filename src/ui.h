#pragma once
#include <api.h>

struct FlightPosition {
    int x;
    int y;
    bool inRange;
};

void ui_init();
FlightPosition calculate_flight_positions(const Flight& f, double currentLat, double currentLong);
void ui_draw_flights(const std::vector<Flight>& flights, double currentLat, double currentLong);
void ui_clear();
FlightPosition calculate_flight_positions(const Flight& f, double currentLat, double currentLong);