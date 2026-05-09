#include "ui.h"
#include <TFT_eSPI.h>
#include <api.h>
#include <math.h>
#include <vector>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite radar = TFT_eSprite(&tft);

static int yOffset;
const int RADAR_SIZE = 128;

void ui_init() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    radar.setColorDepth(16);
    radar.createSprite(128, 128);

    ui_clear();
    yOffset = (tft.height() - RADAR_SIZE) / 2;
    radar.pushSprite(0, yOffset);
}

FlightPosition calculate_flight_positions(const Flight& f, double currentLat, double currentLong) {
    FlightPosition currentFlight;

    const double R = 6371000.0;

    double currentLatRad = currentLat * PI / 180;
    double currentLonRad = currentLong * PI / 180;
    double flightLatRad = f.lat * PI / 180;
    double flightLonRad = f.lon * PI / 180;

    double a = sin((flightLatRad - currentLatRad) / 2) * sin((flightLatRad - currentLatRad) / 2) +
        cos(currentLatRad) * cos(flightLatRad) *
        sin((flightLonRad - currentLonRad) / 2) *  sin((flightLonRad - currentLonRad) / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    double distance = R * c;

    double y = sin(flightLonRad - currentLonRad) * cos(flightLatRad);
    double x = cos(currentLatRad) * sin(flightLatRad) - sin(currentLatRad) * cos(flightLatRad) * cos(flightLonRad - currentLonRad); 
    double bearing = atan2(y, x);

    double maxRange = 30000;
    double normalized = distance / maxRange;
    currentFlight.inRange = (normalized <= 1.0);

    double centerX = 64;
    double centerY = 64;
    double radiusPx = normalized * 60;
    double px = centerX + radiusPx * sin(bearing);
    double py = centerY - radiusPx * cos(bearing);

    currentFlight.x = px;
    currentFlight.y = py;

    return currentFlight;
}

void ui_draw_flights(const std::vector<Flight>& flights, double currentLat, double currentLong) {
    ui_clear();
    for (int i = 0; i < flights.size(); ++i) {
        FlightPosition flightPos = calculate_flight_positions(flights.at(i), currentLat, currentLong);
        if (flightPos.inRange) {
            radar.fillCircle(flightPos.x, flightPos.y, 2, TFT_RED);
        }
    }
    radar.pushSprite(0, yOffset);
}

void ui_clear() {
    radar.fillSprite(TFT_BLACK);
    radar.drawCircle(64, 64, 60, TFT_DARKGREEN);       // outer ring
    radar.drawCircle(64, 64, 40, TFT_DARKGREEN);
    radar.drawCircle(64, 64, 20, TFT_DARKGREEN);
    radar.drawLine(64, 0, 64, 127, TFT_DARKGREEN);     // crosshair
    radar.drawLine(0, 64, 127, 64, TFT_DARKGREEN);
    radar.fillCircle(64, 64, 2, TFT_WHITE);   
}

void ui_tick() {

}