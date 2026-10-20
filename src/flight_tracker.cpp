#include "flight_tracker.h"
#include "flight_geometry.h"
#include <stdlib.h>

FlightTracker::FlightTracker(double latitude, double longitude)
    : latitude_(latitude), longitude_(longitude) {}

bool FlightTracker::refresh() {
  // Fetch new data, keeping the current flights if the request fails.
  FlightFetchResult fetchResult = getFlights(latitude_, longitude_);
  if (!fetchResult.success) {
    stale_ = true;
    return false;
  }

  // Remember the selected aircraft before replacing the flight list.
  String previouslySelectedIcao24;
  if (selectedIndex_ >= 0 && selectedIndex_ < int(flights_.size())) {
    previouslySelectedIcao24 = flights_[selectedIndex_].icao24;
  }

  flights_.swap(fetchResult.flights);
  stale_ = false;

  // Restore the same aircraft, falling back to the first flight in range.
  selectedIndex_ = -1;
  for (size_t flightIndex = 0; flightIndex < flights_.size(); ++flightIndex) {
    const Flight& flight = flights_[flightIndex];
    const auto radarPosition = calculate_flight_positions(flight, latitude_, longitude_);
    if (!radarPosition.inRange) {
      continue;
    }

    if (selectedIndex_ == -1) {
      selectedIndex_ = int(flightIndex);
    }

    if (!previouslySelectedIcao24.isEmpty() && flight.icao24 == previouslySelectedIcao24) {
      selectedIndex_ = int(flightIndex);
      break;
    }
  }

  return true;
}

void FlightTracker::moveSelection(int directionX, int directionY) {
  if (!selectedFlight()) return;
  const auto current = calculate_flight_positions(flights_[selectedIndex_], latitude_, longitude_);
  int bestIndex = -1;
  int bestScore = 0;
  for (size_t i = 0; i < flights_.size(); ++i) {
    if (int(i) == selectedIndex_) continue;
    const auto candidate = calculate_flight_positions(flights_[i], latitude_, longitude_);
    if (!candidate.inRange) continue;
    const int dx = candidate.x - current.x;
    const int dy = candidate.y - current.y;
    const int along = dx * directionX + dy * directionY;
    if (along <= 0) continue;
    const int perpendicular = abs(dx * directionY - dy * directionX);
    const int score = along + perpendicular * 2;
    if (bestIndex == -1 || score < bestScore) {
      bestIndex = int(i);
      bestScore = score;
    }
  }
  if (bestIndex != -1) selectedIndex_ = bestIndex;
}

const std::vector<Flight>& FlightTracker::flights() const { return flights_; }

const Flight* FlightTracker::selectedFlight() const {
  if (selectedIndex_ < 0 || selectedIndex_ >= int(flights_.size())) return nullptr;
  return &flights_[selectedIndex_];
}

int FlightTracker::selectedIndex() const { return selectedIndex_; }
bool FlightTracker::stale() const { return stale_; }
