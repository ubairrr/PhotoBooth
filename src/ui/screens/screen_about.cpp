#include "screen_about.h"
#include "theme.h"
#include "ui_helpers.h"
#include "../../core/app.h"

namespace ui {

static constexpr int SCREEN_W = 240;
static constexpr int SCREEN_H = 320;
static constexpr int HEADER_H = 42;

static void on_back_click(lv_event_t* e) {
    (void)e;
    app_set_state(STATE_SETTINGS);
}

lv_obj_t* ScreenAbout::create() {
    root = lv_obj_create(NULL);
    lv_obj_add_style(root, &Theme::screen_bg, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    buildHeader();
    buildContent();

    return root;
}

void ScreenAbout::destroy() {
    if (root) {
        lv_obj_del(root);
        root = nullptr;
        content = nullptr;
    }
}

void ScreenAbout::buildHeader() {
    lv_obj_t* header = lv_obj_create(root);
    lv_obj_remove_style_all(header);
    lv_obj_add_style(header, &Theme::top_bar, 0);
    lv_obj_set_size(header, SCREEN_W, HEADER_H);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    create_label(header, &Theme::text_cursive, "About", LV_ALIGN_LEFT_MID);

    lv_obj_t* btn_back = create_button(header,
                                       &Theme::btn_secondary,
                                       34,
                                       34,
                                       LV_ALIGN_RIGHT_MID);
    create_label(btn_back, &Theme::text_body, "<", LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_back, on_back_click, LV_EVENT_CLICKED, NULL);
}

void ScreenAbout::buildContent() {
    content = lv_obj_create(root);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, SCREEN_W, SCREEN_H - HEADER_H);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_style_pad_gap(content, 10, 0);
    // Allow scrolling as we have a lot of text
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Using text_small to ensure it fits and looks clean
    lv_obj_t* label = lv_label_create(content);
    lv_obj_add_style(label, &Theme::text_body, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, SCREEN_W - 20);

    lv_label_set_text(label,
        "Created by: Ubairrr\n"
        "Frontend made on LVGL\n\n"
        "Hardware Specs:\n"
        "ESP32-S3 16MB Flash, 8MB RAM.\n"
        "Display: ILI9341 240x320\n\n"
        "Pin Mapping:\n"
        "Camera:\n"
        "  XCLK:15, PCLK:13, VSYNC:6\n"
        "  HREF:7, SDA:4, SCL:5\n"
        "  D0-D7: 11,9,8,10,12,18,17,16\n\n"
        "Display:\n"
        "  MOSI:14, SCLK:21, MISO:1\n"
        "  CS:41, DC:42, RST:47, TOUCH:2\n\n"
        "SD Card (SDMMC):\n"
        "  CMD:38, CLK:39, D0:40"
    );
}

} // namespace ui
