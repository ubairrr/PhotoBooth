#include "screen_settings.h"
#include "theme.h"
#include "ui_helpers.h"
#include "../../core/app.h"
#include <stdio.h>

namespace ui {

static constexpr int SCREEN_W = 240;
static constexpr int SCREEN_H = 320;
static constexpr int HEADER_H = 42;

static void on_back_click(lv_event_t* e) {
    (void)e;
    app_set_state(STATE_VIEWFINDER);
}

static void on_filters_click(lv_event_t* e) {
    (void)e;
    app_set_state(STATE_FILTERS);
}

static void on_countdown_click(lv_event_t* e) {
    static const char* labels[] = {"Off", "3s", "5s", "10s"};
    static const int values[] = {0, 3, 5, 10};

    int index = 0;
    for (int i = 0; i < 4; i++) {
        if (values[i] == app_timer_duration) {
            index = i;
            break;
        }
    }

    index = (index + 1) % 4;
    app_timer_duration = values[index];

    lv_obj_t* value_label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    if (value_label) {
        lv_label_set_text(value_label, labels[index]);
    }
}

static void on_photobooth_capture_click(lv_event_t* e) {
    app_photobooth_style_capture = !app_photobooth_style_capture;

    lv_obj_t* value_label = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
    if (value_label) {
        lv_label_set_text(value_label, app_photobooth_style_capture ? "On" : "Off");
    }
}

static void on_about_click(lv_event_t* e) {
    (void)e;
    app_set_state(STATE_ABOUT);
}

static lv_obj_t* make_row(lv_obj_t* parent,
                          const char* label,
                          const char* value,
                          lv_event_cb_t callback) {
    lv_obj_t* row = lv_btn_create(parent);
    lv_obj_add_style(row, &Theme::card, 0);
    lv_obj_set_size(row, SCREEN_W - 20, 44);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

    create_label(row, &Theme::text_body, label, LV_ALIGN_LEFT_MID);

    lv_obj_t* value_label = create_label(row,
                                         &Theme::text_body,
                                         value,
                                         LV_ALIGN_RIGHT_MID);
    lv_obj_set_style_text_color(value_label, Colors::PRIMARY, 0);

    if (callback) {
        lv_obj_add_event_cb(row, callback, LV_EVENT_CLICKED, value_label);
    }

    return row;
}

lv_obj_t* ScreenSettings::create() {
    root = lv_obj_create(NULL);
    lv_obj_add_style(root, &Theme::screen_bg, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    buildHeader();
    buildContent();

    return root;
}

void ScreenSettings::destroy() {
    if (root) {
        lv_obj_del(root);
        root = nullptr;
        content = nullptr;
    }
}

void ScreenSettings::buildHeader() {
    lv_obj_t* header = lv_obj_create(root);
    lv_obj_remove_style_all(header);
    lv_obj_add_style(header, &Theme::top_bar, 0);
    lv_obj_set_size(header, SCREEN_W, HEADER_H);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    create_label(header, &Theme::text_cursive, "Settings", LV_ALIGN_LEFT_MID);

    lv_obj_t* btn_back = create_button(header,
                                       &Theme::btn_secondary,
                                       34,
                                       34,
                                       LV_ALIGN_RIGHT_MID);
    create_label(btn_back, &Theme::text_body, "<", LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_back, on_back_click, LV_EVENT_CLICKED, NULL);
}

void ScreenSettings::buildContent() {
    content = lv_obj_create(root);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, SCREEN_W, SCREEN_H - HEADER_H);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(content, 10, 0);
    lv_obj_set_style_pad_gap(content, 10, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    char timer_val[8];
    if (app_timer_duration == 0) {
        strcpy(timer_val, "Off");
    } else {
        snprintf(timer_val, sizeof(timer_val), "%ds", app_timer_duration);
    }
    make_row(content, "Countdown", timer_val, on_countdown_click);
    make_row(content, "Filter", "Original", on_filters_click);
    make_row(content, "Photobooth Style", app_photobooth_style_capture ? "On" : "Off", on_photobooth_capture_click);
    make_row(content, "About", "PhotoBooth", on_about_click);
}

} // namespace ui
