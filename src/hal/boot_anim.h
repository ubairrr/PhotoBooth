#pragma once
#include <stdint.h>
// =============================================================================
// hal/boot_anim.h — MJPEG boot animation played directly via TFT_eSPI
//                   (runs before lv_init, no LVGL involvement)
// =============================================================================

/**
 * Stream and render an MJPEG file from the SD card to the display.
 *
 * Call this after display_init() but BEFORE lv_init().
 *
 * @param fps    Target playback frame-rate (default 15)
 */
void boot_anim_play(uint8_t fps = 15);
