#include "ui.h"
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite radar = TFT_eSprite(&tft);

void ui_init() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    radar.setColorDepth(16);
    radar.createSprite(128, 128);
    radar.fillSprite(TFT_BLACK);
    radar.drawCircle(64, 64, 60, TFT_DARKGREEN);       // outer ring
    radar.drawCircle(64, 64, 40, TFT_DARKGREEN);
    radar.drawCircle(64, 64, 20, TFT_DARKGREEN);
    radar.drawLine(64, 0, 64, 127, TFT_DARKGREEN);     // crosshair
    radar.drawLine(0, 64, 127, 64, TFT_DARKGREEN);
    radar.fillCircle(64, 64, 2, TFT_WHITE);   
    radar.pushSprite(0, 0);
}

void ui_draw_flight() {

}

void ui_clear() {

}

void ui_tick() {

}