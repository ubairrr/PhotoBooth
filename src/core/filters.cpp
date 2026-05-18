#include "filters.h"
#include <string.h>

FilterType current_filter = FilterType::NONE;

// Helper to convert RGB565 to RGB888 components
static inline void rgb565_to_888(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = (c >> 11) & 0x1F;
    g = (c >> 5) & 0x3F;
    b = c & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
}

// Helper to convert RGB888 to RGB565
static inline uint16_t rgb888_to_565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void apply_filter(lv_color_t* dst, const lv_color_t* src, int count, FilterType filter) {
    if (filter == FilterType::NONE) {
        memcpy(dst, src, count * sizeof(lv_color_t));
        return;
    }

    for (int i = 0; i < count; i++) {
        uint16_t p = src[i].full;
        
        if (filter == FilterType::INVERT) {
            dst[i].full = ~p; // extremely fast bitwise invert
            continue;
        }

        uint8_t r, g, b;
        rgb565_to_888(p, r, g, b);

        switch (filter) {
            case FilterType::GRAYSCALE: {
                // Luma = 0.299*R + 0.587*G + 0.114*B
                // Fast integer approx: (R*3 + G*4 + B*1) >> 3
                uint8_t luma = (r * 3 + g * 4 + b) >> 3;
                dst[i].full = rgb888_to_565(luma, luma, luma);
                break;
            }
            case FilterType::SEPIA: {
                // Fast approx for sepia
                uint16_t tr = (r * 39 + g * 77 + b * 19) / 100;
                uint16_t tg = (r * 35 + g * 68 + b * 17) / 100;
                uint16_t tb = (r * 27 + g * 53 + b * 13) / 100;
                r = tr > 255 ? 255 : tr;
                g = tg > 255 ? 255 : tg;
                b = tb > 255 ? 255 : tb;
                dst[i].full = rgb888_to_565(r, g, b);
                break;
            }
            case FilterType::WARM: {
                // Boost red, reduce blue
                uint16_t tr = r + 30;
                uint16_t tb = b > 20 ? b - 20 : 0;
                r = tr > 255 ? 255 : tr;
                dst[i].full = rgb888_to_565(r, g, tb);
                break;
            }
            case FilterType::COOL: {
                // Boost blue, reduce red
                uint16_t tb = b + 30;
                uint16_t tr = r > 20 ? r - 20 : 0;
                b = tb > 255 ? 255 : tb;
                dst[i].full = rgb888_to_565(tr, g, b);
                break;
            }
            case FilterType::VIVID: {
                // Increase contrast and boost saturation
                int avg = (r + g + b) / 3;
                auto sat_boost = [&](uint8_t val) -> uint8_t {
                    int v = (int)val;
                    v = avg + ((v - avg) * 15) / 10; // 1.5x saturation boost relative to average
                    v = ((v - 128) * 12) / 10 + 128; // 1.2x contrast
                    return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
                };
                dst[i].full = rgb888_to_565(sat_boost(r), sat_boost(g), sat_boost(b));
                break;
            }
            case FilterType::BLUSH: {
                // Add pink tint (boost red and blue, slightly reduce green)
                uint16_t tr = r + 40;
                uint16_t tb = b + 25;
                uint16_t tg = g > 15 ? g - 15 : 0;
                r = tr > 255 ? 255 : tr;
                b = tb > 255 ? 255 : tb;
                dst[i].full = rgb888_to_565(r, tg, b);
                break;
            }
            default:
                dst[i].full = p;
                break;
        }
    }
}
