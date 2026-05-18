// =============================================================================
// main.cpp — Orchestrator only. All logic lives in hal/, core/, ui/ modules.
// =============================================================================
#include <Arduino.h>
#include <lvgl.h>

#include "hal/display.h"
#include "hal/touch.h"
#include "hal/camera.h"
#include "hal/storage.h"
#include "hal/wifi_share.h"
#include "hal/boot_anim.h"

#include "ui/ui_manager.h"
#include "ui/theme.h"

#include "core/app.h"
#include "core/filters.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== Maryam's Booth — Booting ===");

    if (psramFound()) {
        Serial.printf("PSRAM OK: %u bytes\n", ESP.getPsramSize());
    } else {
        Serial.println("[WARN] PSRAM NOT FOUND");
    }

    // 1. Hardware abstraction layer
    display_init();
    touch_init();
    camera_init();      // allocates canvas_buf + preview_buf in PSRAM
    storage_init();     // mounts SD card

    // 1b. Boot animation — plays BEFORE lv_init() using TFT_eSPI directly
    boot_anim_play();

    // 2. LVGL
    lv_init();
    display_lvgl_init();    // register display driver
    touch_lvgl_register();  // register indev driver

    // 3. UI — must be after lv_init()
    ui::Theme::init();
    ui::UIManager::get().init();

    // Point the LVGL canvas at canvas_buf (live viewfinder double-buffer)
    ui::UIManager::get().mainScreen.setCanvasBuffer(
        reinterpret_cast<uint8_t*>(canvas_buf),
        DISP_W,
        DISP_H
    );

    // 4. App state machine
    app_init();

    // 5. Camera task on Core 0 — streams live frames into preview_buf
    camera_start_task();

    Serial.println("=== Ready ===");
}

void loop() {
    // Core 1: blit the newest live frame from preview_buf → canvas_buf for LVGL.
    // This only runs when we are in the viewfinder state so we don't stomp
    // on captured_buf while the preview screen is active.
    if (app_get_state() == STATE_VIEWFINDER &&
        frame_ready && canvas_buf && preview_buf)
    {
        apply_filter(canvas_buf, preview_buf, DISP_W * DISP_H, current_filter);
        ui::UIManager::get().mainScreen.updateCanvas();
        frame_ready = false;
    }

    // Async capture: once camera_task has filled captured_buf, refresh canvas
    if (app_get_state() == STATE_PREVIEW && camera_has_capture() && !capture_filtered) {
        apply_filter(captured_buf, captured_buf, DISP_W * DISP_H, current_filter);
        capture_filtered = true;
        ui::UIManager::get().previewScreen.showCapturedFrame();
    }

    lv_timer_handler();
    delay(5);
}
