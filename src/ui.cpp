#include "ui.h"
#include "flight_geometry.h"
#include <TFT_eSPI.h>
#include <math.h>

namespace {
TFT_eSPI tft;
TFT_eSprite radar(&tft);
bool spriteReady = false;

using namespace RadarGeometry;

constexpr uint16_t BG = 0x08A3;
constexpr uint16_t PANEL = 0x1126;
constexpr uint16_t GRID = 0x21A8;
constexpr uint16_t TICKS = 0x4B0E;
constexpr uint16_t MUTED = 0x8D57;
constexpr uint16_t TEXT = 0xDF5E;
constexpr uint16_t CYAN = 0x66BD;
constexpr uint16_t AMBER = 0xFE4E;

String callsign(const Flight& flight) {
    String value = flight.callsign;
    value.trim();
    if (value.isEmpty()) {
        value = flight.icao24;
    }
    return value.isEmpty() ? String("UNKNOWN") : value.substring(0, 8);
}

String formatMeasurement(float value, const char* unit) {
    return isfinite(value) ? String(value, 0) + unit : String("--");
}

void label(const String& value, int x, int y, uint16_t color = TEXT) {
    tft.setTextColor(color, BG);
    tft.drawString(value, x, y, 1);
}

void drawPlane(int centerX, int centerY, float heading, uint16_t color) {
    const float headingDegrees = isfinite(heading) ? heading : 0;
    const float headingRadians = headingDegrees * PI / 180.0f;
    const float sinHeading = sinf(headingRadians);
    const float cosHeading = cosf(headingRadians);

    // Aircraft outline relative to its center, pointing north.
    constexpr int vertexCount = 4;
    const int localX[vertexCount] = {0, -3, 0, 3};
    const int localY[vertexCount] = {-4, 4, 2, 4};
    int screenX[vertexCount];
    int screenY[vertexCount];

    // Rotate each vertex to the heading, then move it to the radar position.
    for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
        screenX[vertexIndex] = lroundf(
            centerX
            + localX[vertexIndex] * cosHeading
            - localY[vertexIndex] * sinHeading);
        screenY[vertexIndex] = lroundf(
            centerY
            + localX[vertexIndex] * sinHeading
            + localY[vertexIndex] * cosHeading);
    }

    // Two filled triangles form the aircraft silhouette.
    radar.fillTriangle(
        screenX[0], screenY[0],
        screenX[1], screenY[1],
        screenX[2], screenY[2], color);
    radar.fillTriangle(
        screenX[0], screenY[0],
        screenX[2], screenY[2],
        screenX[3], screenY[3], color);
}

void drawSelectionBrackets(int x, int y) {
    for (int dx : {-1, 1}) {
        for (int dy : {-1, 1}) {
            radar.drawFastHLine(x + dx * 8 - (dx > 0 ? 3 : 0), y + dy * 8, 4, AMBER);
            radar.drawFastVLine(x + dx * 8, y + dy * 8 - (dy > 0 ? 3 : 0), 4, AMBER);
        }
    }
}

}

void ui_init() {
    tft.init();
    tft.setRotation(0);
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.fillScreen(BG);

    radar.setColorDepth(16);
    spriteReady = radar.createSprite(128, 122) != nullptr;
    if (!spriteReady) {
        label("DISPLAY MEMORY", 4, 64, AMBER);
        return;
    }

    ui_draw_flights({}, 0, 0, -1, true);
}

void ui_clear_screen() {
    tft.fillScreen(BG);
}

void ui_clear_sprite() {
    if (spriteReady) {
        radar.fillSprite(BG);
    }
}

void ui_draw_radar() {
    if (!spriteReady) {
        return;
    }

    ui_clear_sprite();

    for (int radius : {17, 33, 50}) {
        radar.drawCircle(CENTER_X, CENTER_Y, radius, GRID);
    }

    for (int offset = -45; offset <= 45; offset += 5) {
        radar.drawPixel(CENTER_X + offset, CENTER_Y, GRID);
        radar.drawPixel(CENTER_X, CENTER_Y + offset, GRID);
    }

    for (int degrees = 0; degrees < 360; degrees += 45) {
        float angle = degrees * PI / 180.0f;
        radar.drawLine(lround(CENTER_X + 47 * sinf(angle)), lround(CENTER_Y - 47 * cosf(angle)),
                       lround(CENTER_X + 50 * sinf(angle)), lround(CENTER_Y - 50 * cosf(angle)), TICKS);
    }

    radar.setTextSize(1);
    radar.setTextDatum(TL_DATUM);
    radar.setTextColor(MUTED, BG);
    radar.drawString("N", 61, 1, 1);
    radar.drawString("S", 61, 112, 1);
    radar.drawString("W", 3, 57, 1);
    radar.drawString("E", 119, 57, 1);
    radar.drawString("100 km", 86, 105, 1);

    radar.drawCircle(CENTER_X, CENTER_Y, 3, TEXT);
    radar.fillCircle(CENTER_X, CENTER_Y, 1, TEXT);
}

void ui_draw_flights(const std::vector<Flight>& flights, double latitude, double longitude, int selectedIndex, bool stale) {
    if (!spriteReady) {
        return;
    }

    ui_draw_radar();

    int visibleFlightCount = 0;
    bool selectedFlightVisible = false;
    for (size_t flightIndex = 0; flightIndex < flights.size(); ++flightIndex) {
        const Flight& flight = flights[flightIndex];
        const FlightPosition position =
            calculate_flight_positions(flight, latitude, longitude);

        if (!position.inRange) {
            continue;
        }

        ++visibleFlightCount;

        // Draw the selected aircraft last so it appears above other aircraft.
        if (int(flightIndex) == selectedIndex) {
            selectedFlightVisible = true;
            continue;
        }

        drawPlane(position.x, position.y, flight.heading, CYAN);
    }

    if (selectedFlightVisible) {
        const Flight& selectedFlight = flights[selectedIndex];
        const FlightPosition position =
            calculate_flight_positions(selectedFlight, latitude, longitude);

        drawSelectionBrackets(position.x, position.y);
        drawPlane(position.x, position.y, selectedFlight.heading, AMBER);
    }

    radar.pushSprite(0, 16);

    tft.fillRect(0, 0, 128, 16, BG);
    label("AIRSPACE", 5, 4);
    tft.setTextDatum(TR_DATUM);
    label(stale ? String("STALE") : String(visibleFlightCount) + " AC", 123, 4, stale ? AMBER : MUTED);
    tft.setTextDatum(TL_DATUM);
    tft.drawFastHLine(4, 15, 120, GRID);

    tft.fillRect(0, 138, 128, 22, PANEL);
    tft.drawFastHLine(0, 138, 128, TICKS);
    tft.fillRect(0, 139, 2, 21, AMBER);

    tft.setTextColor(AMBER, PANEL);
    String title;
    if (selectedFlightVisible) {
        title = callsign(flights[selectedIndex]);
    } else if (stale) {
        title = "DATA UNAVAILABLE";
    } else if (visibleFlightCount > 0) {
        title = "NO SELECTION";
    } else {
        title = "NO TRAFFIC";
    }

    tft.drawString(title, 6, 141, 1);
    if (selectedFlightVisible) {
        tft.setTextColor(MUTED, PANEL);
        tft.drawString("PRESS:INFO", 65, 141, 1);
    }

    tft.setTextColor(TEXT, PANEL);
    if (selectedFlightVisible) {
        tft.drawString(formatMeasurement(flights[selectedIndex].altitude, " m"), 6, 151, 1);
        tft.setTextDatum(TR_DATUM);
        tft.drawString(formatMeasurement(flights[selectedIndex].velocity, " m/s"), 123, 151, 1);
    } else {
        String hint;
        if (stale) {
            hint = "RETRYING...";
        } else if (visibleFlightCount > 0) {
            hint = "MOVE TO SELECT";
        } else {
            hint = "100 km RANGE";
        }

        tft.drawString(hint, 6, 151, 1);
    }

    tft.setTextDatum(TL_DATUM);
}

void ui_draw_details_page(const Flight& flight) {
    ui_clear_screen();

    label(callsign(flight), 6, 7, AMBER);
    label("FLIGHT DETAILS", 6, 20, MUTED);
    tft.drawFastHLine(6, 33, 116, GRID);

    label("ORIGIN", 6, 41, MUTED);
    String country = flight.originCountry;
    if (country.isEmpty()) {
        country = "Unknown";
    }
    label(country.substring(0, 19), 6, 52);
    label(country.substring(19, 38), 6, 62);

    label("ALTITUDE", 6, 79, MUTED);
    label(formatMeasurement(flight.altitude, " m"), 6, 90);

    label("SPEED", 6, 107, MUTED);
    label(formatMeasurement(flight.velocity, " m/s"), 6, 118);
    label("HDG", 82, 107, MUTED);
    label(formatMeasurement(flight.heading, ""), 82, 118);

    tft.fillRect(0, 140, 128, 20, PANEL);
    tft.setTextColor(MUTED, PANEL);
    tft.drawString("PRESS: BACK", 6, 146, 1);
}
