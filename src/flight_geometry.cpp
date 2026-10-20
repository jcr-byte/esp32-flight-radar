#include "flight_geometry.h"
#include <math.h>

using namespace RadarGeometry;

FlightPosition calculate_flight_positions(const Flight& flight, double latitude, double longitude) {
    if (!isfinite(flight.lat) || !isfinite(flight.lon)) {
        return {0, 0, false};
    }

    const double radarLatitudeRadians = latitude * PI / 180;
    const double flightLatitudeRadians = flight.lat * PI / 180;
    const double deltaLatitudeRadians = flightLatitudeRadians - radarLatitudeRadians;
    const double deltaLongitudeRadians = (flight.lon - longitude) * PI / 180;

    // Calculate the great-circle distance using the haversine formula.
    constexpr double earthRadiusMeters = 6371000.0;
    double haversine = pow(sin(deltaLatitudeRadians / 2), 2)
        + cos(radarLatitudeRadians) * cos(flightLatitudeRadians)
        * pow(sin(deltaLongitudeRadians / 2), 2);
    // Rounding can push this outside [0, 1], making the square roots invalid.
    haversine = fmax(0.0, fmin(1.0, haversine));
    const double distanceMeters = earthRadiusMeters * 2
        * atan2(sqrt(haversine), sqrt(1 - haversine));

    // Bearing is measured clockwise from north.
    const double bearingRadians = atan2(
        sin(deltaLongitudeRadians) * cos(flightLatitudeRadians),
        cos(radarLatitudeRadians) * sin(flightLatitudeRadians)
            - sin(radarLatitudeRadians) * cos(flightLatitudeRadians) * cos(deltaLongitudeRadians));

    const double radiusPixels = distanceMeters / MAX_RANGE * RADIUS;
    const int screenX = int(lround(CENTER_X + radiusPixels * sin(bearingRadians)));
    // Screen y increases downward, so subtract the northward offset.
    const int screenY = int(lround(CENTER_Y - radiusPixels * cos(bearingRadians)));
    const bool inRange = distanceMeters <= MAX_RANGE;
    return {screenX, screenY, inRange};
}

int calculateRadiusDistances(int radius) {
    return lround(radius * MAX_RANGE / RADIUS / 1000);
}

