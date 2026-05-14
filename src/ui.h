#pragma once
#include <api.h>

struct FlightPosition {
    int x;
    int y;
    bool inRange;
};

void ui_init();
FlightPosition calculate_flight_positions(const Flight& f, double currentLat, double currentLong);
void ui_draw_flights(const std::vector<Flight>& flights, double currentLat, double currentLong, int currentIndex);
void ui_clear_screen();
void ui_clear_sprite();
int calculateRadiusDistances(int ringRadi);
FlightPosition calculate_flight_positions(const Flight& f, double currentLat, double currentLong);
void ui_draw_radar();
void ui_draw_details_page(Flight& f);