#pragma once
// =============================================================================
// hal/camera.h — OV3660 live viewfinder + PSRAM capture cache
// =============================================================================
#include <Arduino.h>
#include <lvgl.h>
#include "esp_camera.h"

#define DISP_W 240
#define DISP_H 320

// ---------------------------------------------------------------------------
// Shared buffers  (written by camera_task on Core 0, read on Core 1)
// ---------------------------------------------------------------------------
extern lv_color_t* preview_buf;     // camera task writes live frames here
extern lv_color_t* canvas_buf;      // main loop copies preview_buf here for LVGL

// ---------------------------------------------------------------------------
// Capture cache  (PSRAM snapshot of the frozen frame)
// ---------------------------------------------------------------------------
extern lv_color_t* captured_buf;    // non-null when a photo is cached
extern int         captured_w;
extern int         captured_h;

// ---------------------------------------------------------------------------
// Inter-core flags
// ---------------------------------------------------------------------------
extern volatile bool frame_ready;       // new live frame is ready to blit
extern volatile bool capture_requested; // signal camera task to freeze a frame
extern volatile bool capture_filtered;  // true if filter has been applied to capture cache

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void camera_init();
void camera_start_task();                               // launches task on Core 0
void camera_task(void* pvParameters);
void copy_portrait_frame(camera_fb_t* fb, lv_color_t* dst);

/**
 * Freeze the current live frame into captured_buf (PSRAM).
 * Sets capture_requested = true so the camera task grabs the next frame
 * and copies it into captured_buf.
 * Non-blocking — the caller should poll camera_has_capture() or transition
 * to the preview screen and let the task fill the buffer asynchronously.
 */
void camera_capture();

/** Returns true once captured_buf has been filled after a camera_capture() call. */
bool camera_has_capture();

/**
 * Free captured_buf and reset captured_w/h.
 * Must be called from Core 1 (LVGL task) only when LVGL is not referencing it.
 */
void camera_clear_cache();
