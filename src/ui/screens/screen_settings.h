#pragma once
#include "lvgl.h"

namespace ui {

class ScreenSettings {
public:
    lv_obj_t* create();
    void destroy();

private:
    lv_obj_t* root = nullptr;
    lv_obj_t* content = nullptr;

    void buildHeader();
    void buildContent();
};

}
