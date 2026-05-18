// =============================================================================
// hal/display.cpp — TFT_eSPI init + LVGL flush callback
// =============================================================================
#include "display.h"
#include "../ui/theme.h"

TFT_eSPI tft = TFT_eSPI();

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;

void display_init() {
    // Hardware reset
    pinMode(47, OUTPUT);
    digitalWrite(47, LOW);  delay(100);
    digitalWrite(47, HIGH); delay(100);

    tft.init();
    tft.setRotation(0);          // Portrait 240x320
    tft.fillScreen(TFT_BLACK);

#if defined(TOUCH_CALIBRATE_ON_BOOT)
    uint16_t calData[5];
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("Touch corners", 120, 144, 2);
    tft.drawCentreString("Watch Serial", 120, 164, 2);
    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
    Serial.printf("[TOUCH] calData: { %u, %u, %u, %u, %u }\n",
                  calData[0], calData[1], calData[2], calData[3], calData[4]);
    tft.fillScreen(TFT_BLACK);
#else
    // Portrait calibration profile. Run touch-calibrator if taps feel offset.
    uint16_t calData[5] = { 431, 3329, 500, 3353, 4 };
#endif
    tft.setTouch(calData);

    Serial.println("[DISP] TFT OK — Portrait 240x320");
}

void display_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(drv);
}

void display_lvgl_init() {
    // Allocate 20 rows of double-buffers in PSRAM
    const uint32_t buf_size_px = 240 * 20;
    buf1 = (lv_color_t*)ps_malloc(buf_size_px * sizeof(lv_color_t));
    buf2 = (lv_color_t*)ps_malloc(buf_size_px * sizeof(lv_color_t));

    if (!buf1 || !buf2) {
        Serial.println("[DISP] FATAL: Failed to allocate LVGL draw buffers in PSRAM");
        return;
    }

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_size_px);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = 240;
    disp_drv.ver_res  = 320;
    disp_drv.flush_cb = display_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    Serial.println("[DISP] LVGL display driver registered (240x320, PSRAM buffers)");
}
