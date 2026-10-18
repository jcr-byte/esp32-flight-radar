# Radar screen design

Open [the screen mockup](radar-concept.svg) in a browser. It is drawn on a true 128 × 160 coordinate grid and enlarged 6× for inspection. Flight values and the aircraft count are illustrative. Text uses a browser monospace approximation; use the TFT's built-in 6 × 8 font when implementing and check fit on the device.

The radar should feel like a small aviation instrument: a dark navy field, subdued geometry, clear aircraft silhouettes, and one warm accent for selection. Keep the view north-up so joystick navigation remains predictable.

| Area | Screen coordinates | Purpose |
| --- | --- | --- |
| Header | y = 0–16 | AIRSPACE title and count of aircraft actually visible in range |
| Radar | y = 18–136 | Center (64, 76), outer radius 50 pixels, north-up |
| Selection panel | y = 138–159 | Callsign, altitude in meters, speed in m/s, and press-for-info hint |

| Role | Color |
| --- | --- |
| Background | `#07131F` |
| Panel background | `#102432` |
| Range rings and dotted axes | `#203747` |
| Compass ticks | `#496372` |
| Secondary text | `#8DA8B8` |
| Primary text and home marker | `#DCEAF1` |
| Aircraft | `#63D5E8` |
| Selected aircraft and corner brackets | `#FFCA70` |

Use three rings at 17, 33, and 50 pixels, representing approximately 34, 66, and 100 km. Label only the outer range to reduce clutter. Put compass letters outside the ring. Use compact directional aircraft markers with a notched tail; the selected aircraft also gets four corner brackets so selection is distinguishable without color alone. Draw selection last so nearby traffic cannot obscure it. Keep markers inside the radar's 128 × 120 drawing area, including at the range boundary.

Retain a white home marker at the center. Keep ordinary traffic cyan; red would imply an alert that the current data does not establish. Avoid an animated sweep: the data updates every ten seconds, so a sweep would suggest a scan the app does not perform.

The footer exposes useful information before opening the details page. Trim surrounding callsign whitespace, use the ICAO identifier when the callsign is blank, and show `--` for unavailable numeric values. Restrict callsign text to eight characters to keep it clear of the action hint. With no valid selection, show `NO SELECTION` and `MOVE TO SELECT`; with no aircraft in range, show `NO TRAFFIC` and `100 km RANGE`. Count only in-range aircraft in the header. The header dot is a visual accent in this mockup; in firmware, show it only when data has loaded successfully, and replace it with a subdued stale indicator after a failed refresh.

For the details screen, repeat the navy background and amber callsign header. Stack country, altitude, speed, and heading with muted labels and white values. Wrap long country names and leave a `PRESS: BACK` hint at the bottom.

Implementation exercise: start with the palette and frame, then place the compass and rings, then update flight projection to match the new center and 50-pixel radius. The current projection is relative to a 128 × 128 sprite; either position that sprite consistently with the new layout or use a full-screen sprite and screen coordinates. Keep distance labels and projection tied to the same radius. Add the selection panel last, checking empty data and selection validity before reading a flight. A 128 × 160 sprite at 16 bits per pixel requires 40,960 bytes; check sprite allocation before drawing. Verify text spacing and contrast on the physical TFT.
