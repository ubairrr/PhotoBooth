// =============================================================================
// core/filter_engine.cpp — RGB565 software filters
// =============================================================================
#include "filter_engine.h"

FilterId active_filter = FILTER_ORIGINAL;

static inline uint16_t clamp16(int v) { return (v < 0) ? 0 : (v > 255) ? 255 : v; }

void filter_apply(uint16_t *buf, size_t pixel_count) {
    if (active_filter == FILTER_ORIGINAL) return;

    for (size_t i = 0; i < pixel_count; i++) {
        uint16_t p = buf[i];
        int r = (p >> 11) & 0x1F;
        int g = (p >> 5)  & 0x3F;
        int b =  p        & 0x1F;
        // Scale to 8-bit
        r = (r << 3); g = (g << 2); b = (b << 3);

        switch (active_filter) {
            case FILTER_SWEET_PINK:
                r = clamp16(r + 30); b = clamp16(b - 20); break;
            case FILTER_SOFT_PEACH:
                r = clamp16(r + 20); g = clamp16(g + 10); b = clamp16(b - 10); break;
            case FILTER_DREAMY:
                r = clamp16((r + 255) / 2); g = clamp16((g + 200) / 2); b = clamp16((b + 220) / 2); break;
            case FILTER_BW: {
                int lum = (r * 77 + g * 150 + b * 29) >> 8;
                r = g = b = lum; break;
            }
            case FILTER_VINTAGE:
                r = clamp16(r * 9 / 10 + 20); g = clamp16(g * 9 / 10); b = clamp16(b * 7 / 10); break;
            default: break;
        }

        r >>= 3; g >>= 2; b >>= 3;
        buf[i] = ((uint16_t)r << 11) | ((uint16_t)g << 5) | (uint16_t)b;
    }
}

const char* filter_name(FilterId id) {
    switch (id) {
        case FILTER_ORIGINAL:   return "Original";
        case FILTER_SWEET_PINK: return "Sweet Pink";
        case FILTER_SOFT_PEACH: return "Soft Peach";
        case FILTER_DREAMY:     return "Dreamy";
        case FILTER_BW:         return "B&W";
        case FILTER_VINTAGE:    return "Vintage";
        default:                return "Unknown";
    }
}
