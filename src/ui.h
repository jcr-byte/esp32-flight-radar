#pragma once
#include <api.h>

void ui_init();
void ui_draw_flights(const std::vector<Flight>& flights, double currentLat, double currentLong, int currentIndex, bool stale);
void ui_clear_screen();
void ui_clear_sprite();
void ui_draw_radar();
void ui_draw_details_page(const Flight& f);
