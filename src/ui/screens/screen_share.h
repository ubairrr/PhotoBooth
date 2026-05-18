#pragma once
// =============================================================================
// screen_share.h  —  "Sharing active" screen
//
// Shows the WiFi AP credentials + a QR code that links to the web gallery.
// A "Stop Sharing" button calls wifi_share_stop() and returns to STATE_GALLERY.
// =============================================================================
#include "lvgl.h"

namespace ui {

class ScreenShare {
public:
    lv_obj_t* create();
    void       destroy();

private:
    lv_obj_t* root = nullptr;
};

} // namespace ui
