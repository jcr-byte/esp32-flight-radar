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
  if (const Flight* previousFlight = selectedFlight()) {
    previouslySelectedIcao24 = previousFlight->icao24;
  }

  flights_.swap(fetchResult.flights);
  stale_ = false;

  // Restore the same aircraft, falling back to the first flight in range.
  selectedIndex_ = -1;
  for (size_t flightIndex = 0; flightIndex < flights_.size(); ++flightIndex) {
    const Flight& flight = flights_[flightIndex];
    const FlightPosition radarPosition = calculate_flight_positions(flight, latitude_, longitude_);
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
  const Flight* currentFlight = selectedFlight();
  if (!currentFlight) {
    return;
  }

  const FlightPosition currentPosition =
      calculate_flight_positions(*currentFlight, latitude_, longitude_);

  int bestFlightIndex = -1;
  int bestScore = 0;
  constexpr int sidewaysPenalty = 2;

  for (size_t flightIndex = 0; flightIndex < flights_.size(); ++flightIndex) {
    if (int(flightIndex) == selectedIndex_) {
      continue;
    }

    const FlightPosition candidatePosition =
        calculate_flight_positions(flights_[flightIndex], latitude_, longitude_);

    if (!candidatePosition.inRange) {
      continue;
    }

    const int offsetX = candidatePosition.x - currentPosition.x;
    const int offsetY = candidatePosition.y - currentPosition.y;

    // Only consider aircraft ahead in the requested direction.
    const int forwardDistance = offsetX * directionX + offsetY * directionY;
    if (forwardDistance <= 0) {
      continue;
    }

    // Prefer nearby aircraft aligned with the requested direction.
    const int sidewaysDistance = abs(offsetX * directionY - offsetY * directionX);
    const int candidateScore =
        forwardDistance + sidewaysDistance * sidewaysPenalty;

    if (bestFlightIndex == -1 || candidateScore < bestScore) {
      bestFlightIndex = int(flightIndex);
      bestScore = candidateScore;
    }
  }

  if (bestFlightIndex != -1) {
    selectedIndex_ = bestFlightIndex;
  }
}

const std::vector<Flight>& FlightTracker::flights() const {
  return flights_;
}

const Flight* FlightTracker::selectedFlight() const {
  if (selectedIndex_ < 0 || selectedIndex_ >= int(flights_.size())) {
    return nullptr;
  }

  return &flights_[selectedIndex_];
}

int FlightTracker::selectedIndex() const {
  return selectedIndex_;
}

bool FlightTracker::stale() const {
  return stale_;
}
