#pragma once
#include "lvgl.h"
#include "../components/cat.h"

namespace ui {

class ScreenMain {
public:
    lv_obj_t* create();
    void destroy();

    // Called from main loop when new frame is ready
    void updateCanvas();
    void setCanvasBuffer(uint8_t* buffer, int w, int h);
    void openPreview();
    void startCountdown();

private:
    lv_obj_t* root = nullptr;

    lv_obj_t* top_bar = nullptr;
    lv_obj_t* camera_container = nullptr;
    lv_obj_t* canvas = nullptr;
    lv_obj_t* bottom_bar = nullptr;

    // UI elements
    lv_obj_t* btn_capture = nullptr;
    lv_obj_t* btn_filters = nullptr;
    lv_obj_t* btn_gallery = nullptr;

    uint8_t* canvasBuffer = nullptr;
    int canvasWidth = 0;
    int canvasHeight = 0;

    Cat top_cat;
    Cat left_cat;
    Cat right_cat;
    Cat top_bow;

    lv_obj_t* timer_label = nullptr;
    lv_obj_t* countdown_label = nullptr; // Large center overlay
    int countdown_value = 0;
    lv_timer_t* countdown_timer = nullptr;

    // Helpers
    void buildTopBar();
    void buildCameraArea();
    void buildBottomBar();

    lv_obj_t* createCaptureButton(lv_obj_t* parent);
    static void countdown_timer_cb(lv_timer_t* t);
};

} // namespace ui
