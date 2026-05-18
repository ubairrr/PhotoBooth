#pragma once
#include "lvgl.h"



namespace ui {

struct Colors {
    static const lv_color_t PRIMARY;
    static const lv_color_t PRIMARY_DARK;
    static const lv_color_t LIGHT;
    static const lv_color_t BG;
    static const lv_color_t TEXT;
    static const lv_color_t WHITE;
};

class Theme {
public:
    static void init();

    // Containers
    static lv_style_t screen_bg;
    static lv_style_t top_bar;
    static lv_style_t bottom_bar;
    static lv_style_t card;

    // Buttons
    static lv_style_t btn_primary;
    static lv_style_t btn_secondary;
    static lv_style_t btn_capture;

    // Text
    static lv_style_t text_title;
    static lv_style_t text_body;
    static lv_style_t text_cursive;

    // Utility
    static lv_style_t focus_frame;
};

} // namespace ui
