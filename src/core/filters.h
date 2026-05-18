#pragma once
#include <lvgl.h>

enum class FilterType {
    NONE,
    GRAYSCALE,
    SEPIA,
    INVERT,
    WARM,
    COOL,
    VIVID,
    BLUSH
};

extern FilterType current_filter;

void apply_filter(lv_color_t* dst, const lv_color_t* src, int count, FilterType filter);
