#pragma once
// =============================================================================
// core/filter_engine.h — Software RGB565 filter processing
// =============================================================================
#include <stdint.h>
#include <stddef.h>

enum FilterId {
    FILTER_ORIGINAL = 0,
    FILTER_SWEET_PINK,
    FILTER_SOFT_PEACH,
    FILTER_DREAMY,
    FILTER_BW,
    FILTER_VINTAGE,
    FILTER_COUNT
};

extern FilterId active_filter;

// Apply active_filter to a RGB565 buffer in-place
void filter_apply(uint16_t *buf, size_t pixel_count);
const char* filter_name(FilterId id);
