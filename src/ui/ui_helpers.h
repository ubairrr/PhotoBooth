#pragma once
#include "lvgl.h"

namespace ui {

// Inline → zero overhead
inline lv_obj_t* create_container(lv_obj_t* parent, lv_style_t* style,
                                  int w, int h, lv_align_t align, int x = 0, int y = 0) {
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_add_style(obj, style, 0);
    lv_obj_set_size(obj, w, h);
    lv_obj_align(obj, align, x, y);
    return obj;
}

inline lv_obj_t* create_button(lv_obj_t* parent, lv_style_t* style,
                              int w, int h, lv_align_t align, int x = 0, int y = 0) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_add_style(btn, style, 0);
    lv_obj_set_size(btn, w, h);
    lv_obj_align(btn, align, x, y);
    return btn;
}

inline lv_obj_t* create_label(lv_obj_t* parent, lv_style_t* style,
                             const char* text, lv_align_t align, int x = 0, int y = 0) {
    lv_obj_t* label = lv_label_create(parent);
    lv_obj_add_style(label, style, 0);
    lv_label_set_text(label, text);
    lv_obj_align(label, align, x, y);
    return label;
}

} // namespace ui
