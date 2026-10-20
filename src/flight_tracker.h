#pragma once
#include "api.h"

class FlightTracker {
public:
    FlightTracker(double latitude, double longitude);

    bool refresh();
    void moveSelection(int directionX, int directionY);

    const std::vector<Flight>& flights() const;
    // Valid until the next successful refresh; do not retain across refreshes.
    const Flight* selectedFlight() const;
    int selectedIndex() const;
    bool stale() const;

private:
    double latitude_;
    double longitude_;
    std::vector<Flight> flights_;
    int selectedIndex_ = -1;
    bool stale_ = true;
};
