#include "ui.h"
#include <TFT_eSPI.h>
#include <api.h>
#include <math.h>
#include <vector>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite radar = TFT_eSprite(&tft);

static int yOffset;
const int RADAR_SIZE = 128;
const double maxRange = 50000;

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

void drawPlaneTriangle(int cx, int cy, float heading, uint16_t color) {

    int size = 5;

    float headingRad = heading * (PI / 180.0f);
    float cosHeading = cosf(headingRad);
    float sinHeading = sinf(headingRad);

    float tipX = 0;
    float tipY = -size;
    float backLeftx = -size * 0.6f;
    float backLefty = size * 1.5f;
    float backRightx = size * 0.6f;
    float backRighty = size * 1.5f;

    float tipXRotated = tipX * cosHeading - tipY * sinHeading;
    float tipYRotated = tipX * sinHeading + tipY * cosHeading;
    float backLeftXRotated = backLeftx * cosHeading - backLefty * sinHeading;
    float backLeftYRotated = backLeftx * sinHeading + backLefty * cosHeading;
    float backRightXRotated = backRightx * cosHeading - backRighty * sinHeading;
    float backRightYRotated = backRightx * sinHeading + backRighty * cosHeading;

    int tipFinalX = lroundf(tipXRotated + cx);
    int tipFinalY = lroundf(tipYRotated + cy);
    int backLeftFinalX = lroundf(backLeftXRotated + cx);
    int backLeftFinalY = lroundf(backLeftYRotated + cy);
    int backRightFinalX = lroundf(backRightXRotated + cx);
    int backRightFinalY = lroundf(backRightYRotated + cy);

    radar.fillTriangle(tipFinalX, tipFinalY, backLeftFinalX, backLeftFinalY, backRightFinalX, backRightFinalY, color);
}

void ui_draw_flights(const std::vector<Flight>& flights, double currentLat, double currentLong, int selectedIndex) {
    ui_clear();
    for (int i = 0; i < flights.size(); ++i) {
        FlightPosition flightPos = calculate_flight_positions(flights.at(i), currentLat, currentLong);
        if (flightPos.inRange) {
            if (i == selectedIndex) {
                drawPlaneTriangle(flightPos.x, flightPos.y, flights.at(i).heading, TFT_GREEN);
            } else {
                drawPlaneTriangle(flightPos.x, flightPos.y, flights.at(i).heading, TFT_RED);
            }
        }
    }
    radar.pushSprite(0, yOffset);
}

int calculateRadiusDistances(int ringRadi) {
    int distance = ringRadi * maxRange / 60;
    return distance / 1000;
}


void ui_clear() {
    radar.fillSprite(TFT_BLACK);
    radar.drawCircle(64, 64, 60, TFT_DARKGREEN);       // outer ring
    radar.drawCircle(64, 64, 40, TFT_DARKGREEN);
    radar.drawCircle(64, 64, 20, TFT_DARKGREEN);
    radar.drawLine(64, 0, 64, 127, TFT_DARKGREEN);     // crosshair
    radar.drawLine(0, 64, 127, 64, TFT_DARKGREEN);
    radar.fillCircle(64, 64, 2, TFT_WHITE);   
    radar.setTextColor(TFT_DARKGREEN, TFT_BLACK);
    radar.setTextSize(1);
    radar.setTextDatum(MC_DATUM);
    
    int radiOne = calculateRadiusDistances(20);
    String labelOne = String(radiOne) + "km";
    radar.drawString(labelOne, 78, 48);
    int radiTwo = calculateRadiusDistances(40);
    String labelTwo = String(radiTwo) + "km";
    radar.drawString(labelTwo, 78, 28);
    int radiThree = calculateRadiusDistances(60);
    String labelThree = String(radiThree) + "km";
    radar.drawString(labelThree, 78, 8);
}


