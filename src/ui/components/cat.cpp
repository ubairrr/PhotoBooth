#include "cat.h"

namespace ui {

static void anim_y_cb(void* var, int32_t v) {
    lv_obj_set_y((lv_obj_t*)var, v);
}

void Cat::create(lv_obj_t* parent,
                 const void* img_src,
                 int x, int y) {

    obj = lv_img_create(parent);
    lv_img_set_src(obj, img_src);

    lv_obj_add_flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_pos(obj, x, y);
    this->base_y = y;
}

void Cat::startFloatAnim(int amplitude, int duration) {
    if (!obj) return;

    // Use the explicitly set base_y instead of asking LVGL for a potentially uncalculated layout coordinate
    // int base_y = lv_obj_get_y(obj);

    lv_anim_t a;
    lv_anim_init(&a);

    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_y_cb);

    lv_anim_set_values(&a,
                       base_y - amplitude,
                       base_y + amplitude);

    lv_anim_set_time(&a, duration);
    lv_anim_set_playback_time(&a, duration);

    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);

    lv_anim_start(&a);
}

void Cat::setAngle(int16_t angle) {
    if (obj) {
        lv_img_set_angle(obj, angle);
    }
}

}
