#pragma once
// =============================================================================
// hal/display.h — TFT_eSPI + LVGL display flush driver
// =============================================================================
#include <TFT_eSPI.h>
#include <lvgl.h>

extern TFT_eSPI tft;

void display_init();
void display_lvgl_init();
void display_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p);
