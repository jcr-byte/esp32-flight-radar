#include "ui.h"
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void ui_init() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
}

void ui_draw_flight() {

}

void ui_clear() {

}

void ui_tick() {

}