#pragma once
#include "lvgl.h"

namespace ui {

class ScreenPreview {
public:
    lv_obj_t* create();
    void destroy();

    /**
     * Bind the canvas to the current captured_buf and refresh it.
     * Call this after navigating to the preview screen (or whenever you want
     * to force a canvas refresh, e.g. after an async capture completes).
     */
    void showCapturedFrame();

    /**
     * Write captured_buf to SD card as a BMP, clear the PSRAM cache,
     * and transition back to STATE_VIEWFINDER.
     */
    bool saveCurrentImage();

private:
    lv_obj_t* root         = nullptr;
    lv_obj_t* canvas       = nullptr;
    lv_obj_t* status_label = nullptr;

    void setStatus(const char* text);
};

} // namespace ui
