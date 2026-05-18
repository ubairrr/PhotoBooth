#pragma once
#include "lvgl.h"

namespace ui {

class Cat {
public:
    void create(lv_obj_t* parent,
                const void* img_src,
                int x, int y);

    void startFloatAnim(int amplitude = 3,
                        int duration = 2000);

    void setAngle(int16_t angle);

private:
    lv_obj_t* obj = nullptr;
    int base_y = 0;
};

} // namespace ui