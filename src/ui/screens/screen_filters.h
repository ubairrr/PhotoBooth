#pragma once
#include "lvgl.h"
#include "../../core/filters.h"
#include <vector>

namespace ui {

class ScreenFilters {
public:
    lv_obj_t* create();
    void destroy();

private:
    lv_obj_t* root = nullptr;
    lv_obj_t* grid = nullptr;
    
    std::vector<lv_img_dsc_t*> filtered_images;

    void buildHeader();
    void buildGrid();

    lv_obj_t* createFilterItem(lv_obj_t* parent, const char* name, FilterType type);
};

}