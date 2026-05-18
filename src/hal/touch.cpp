// =============================================================================
// hal/touch.cpp — XPT2046 LVGL indev driver
// =============================================================================
#include "touch.h"
#include "display.h"

void touch_init() {
    // TFT touch is already set up in display_init() via tft.setTouch()
    Serial.println("[TOUCH] Touch driver ready");
}

void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y, 300)) {
        data->point.x = x;
        data->point.y = y;
        data->state   = LV_INDEV_STATE_PR;
        Serial.printf("[TOUCH] x=%d y=%d\n", x, y);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void touch_lvgl_register() {
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read;
    lv_indev_drv_register(&indev_drv);
    Serial.println("[TOUCH] LVGL indev registered");
}
