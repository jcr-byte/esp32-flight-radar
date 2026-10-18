#include "ui.h"
#include <TFT_eSPI.h>
#include <math.h>

namespace {
TFT_eSPI tft;
TFT_eSprite radar(&tft);
bool spriteReady = false;
constexpr int CENTER_X = 64;
constexpr int CENTER_Y = 60; // sprite starts at screen y=16
constexpr int RADIUS = 50;
constexpr double MAX_RANGE = 100000.0;
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
    if (value.isEmpty()) value = flight.icao24;
    return value.isEmpty() ? String("UNKNOWN") : value.substring(0, 8);
}

String measurement(float value, const char* unit) {
    return isfinite(value) ? String(value, 0) + unit : String("--");
}

void label(const String& value, int x, int y, uint16_t color = TEXT) {
    tft.setTextColor(color, BG);
    tft.drawString(value, x, y, 1);
}

void drawPlane(int cx, int cy, float heading, uint16_t color) {
    const float angle = (isfinite(heading) ? heading : 0) * PI / 180.0f;
    const float s = sinf(angle), c = cosf(angle);
    const int px[] = {0, -3, 0, 3};
    const int py[] = {-4, 4, 2, 4};
    int x[4], y[4];
    for (int i = 0; i < 4; ++i) {
        x[i] = lroundf(cx + px[i] * c - py[i] * s);
        y[i] = lroundf(cy + px[i] * s + py[i] * c);
    }
    radar.fillTriangle(x[0], y[0], x[1], y[1], x[2], y[2], color);
    radar.fillTriangle(x[0], y[0], x[2], y[2], x[3], y[3], color);
}

void brackets(int x, int y) {
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

FlightPosition calculate_flight_positions(const Flight& f, double lat, double lon) {
    if (!isfinite(f.lat) || !isfinite(f.lon)) return {0, 0, false};
    const double lat1 = lat * PI / 180, lat2 = f.lat * PI / 180;
    const double deltaLat = lat2 - lat1, deltaLon = (f.lon - lon) * PI / 180;
    double a = pow(sin(deltaLat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(deltaLon / 2), 2);
    a = fmax(0.0, fmin(1.0, a));
    const double distance = 6371000.0 * 2 * atan2(sqrt(a), sqrt(1 - a));
    const double bearing = atan2(sin(deltaLon) * cos(lat2),
        cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(deltaLon));
    const double radius = distance / MAX_RANGE * RADIUS;
    return {int(lround(CENTER_X + radius * sin(bearing))),
            int(lround(CENTER_Y - radius * cos(bearing))), distance <= MAX_RANGE};
}

int calculateRadiusDistances(int radius) {
    return lround(radius * MAX_RANGE / RADIUS / 1000);
}

void ui_clear_screen() { tft.fillScreen(BG); }
void ui_clear_sprite() { if (spriteReady) radar.fillSprite(BG); }

void ui_draw_radar() {
    if (!spriteReady) return;
    ui_clear_sprite();
    for (int radius : {17, 33, 50}) radar.drawCircle(CENTER_X, CENTER_Y, radius, GRID);
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

void ui_draw_flights(const std::vector<Flight>& flights, double lat, double lon, int selectedIndex, bool stale) {
    if (!spriteReady) return;
    ui_draw_radar();
    int count = 0;
    bool selectedVisible = false;
    for (size_t i = 0; i < flights.size(); ++i) {
        const auto pos = calculate_flight_positions(flights[i], lat, lon);
        if (!pos.inRange) continue;
        ++count;
        if (int(i) == selectedIndex) { selectedVisible = true; continue; }
        drawPlane(pos.x, pos.y, flights[i].heading, CYAN);
    }
    if (selectedVisible) {
        const auto& flight = flights[selectedIndex];
        const auto pos = calculate_flight_positions(flight, lat, lon);
        brackets(pos.x, pos.y);
        drawPlane(pos.x, pos.y, flight.heading, AMBER);
    }
    radar.pushSprite(0, 16);
    tft.fillRect(0, 0, 128, 16, BG);
    label("AIRSPACE", 5, 4);
    tft.setTextDatum(TR_DATUM);
    label(stale ? String("STALE") : String(count) + " AC", 123, 4, stale ? AMBER : MUTED);
    tft.setTextDatum(TL_DATUM);
    tft.drawFastHLine(4, 15, 120, GRID);
    tft.fillRect(0, 138, 128, 22, PANEL);
    tft.drawFastHLine(0, 138, 128, TICKS);
    tft.fillRect(0, 139, 2, 21, AMBER);
    tft.setTextColor(AMBER, PANEL);
    String title = selectedVisible ? callsign(flights[selectedIndex])
        : (stale ? "DATA UNAVAILABLE" : (count ? "NO SELECTION" : "NO TRAFFIC"));
    tft.drawString(title, 6, 141, 1);
    if (selectedVisible) {
        tft.setTextColor(MUTED, PANEL);
        tft.drawString("PRESS:INFO", 65, 141, 1);
    }
    tft.setTextColor(TEXT, PANEL);
    if (selectedVisible) {
        tft.drawString(measurement(flights[selectedIndex].altitude, " m"), 6, 151, 1);
        tft.setTextDatum(TR_DATUM);
        tft.drawString(measurement(flights[selectedIndex].velocity, " m/s"), 123, 151, 1);
    } else {
        tft.drawString(stale ? "RETRYING..." : (count ? "MOVE TO SELECT" : "100 km RANGE"), 6, 151, 1);
    }
    tft.setTextDatum(TL_DATUM);
}

void ui_draw_details_page(Flight& f) {
    ui_clear_screen();
    label(callsign(f), 6, 7, AMBER);
    label("FLIGHT DETAILS", 6, 20, MUTED);
    tft.drawFastHLine(6, 33, 116, GRID);
    label("ORIGIN", 6, 41, MUTED);
    String country = f.originCountry;
    if (country.isEmpty()) country = "Unknown";
    label(country.substring(0, 19), 6, 52);
    label(country.substring(19, 38), 6, 62);
    label("ALTITUDE", 6, 79, MUTED);
    label(measurement(f.altitude, " m"), 6, 90);
    label("SPEED", 6, 107, MUTED);
    label(measurement(f.velocity, " m/s"), 6, 118);
    label("HDG", 82, 107, MUTED);
    label(measurement(f.heading, ""), 82, 118);
    tft.fillRect(0, 140, 128, 20, PANEL);
    tft.setTextColor(MUTED, PANEL);
    tft.drawString("PRESS: BACK", 6, 146, 1);
}
