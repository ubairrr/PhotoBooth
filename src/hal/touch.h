#pragma once
// =============================================================================
// hal/touch.h — XPT2046 touch input driver
// =============================================================================
#include <lvgl.h>

void touch_init();
void touch_lvgl_register();
void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
