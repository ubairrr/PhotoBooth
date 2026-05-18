#include "screen_preview.h"
#include <Arduino.h>
#include "theme.h"
#include "ui_helpers.h"
#include "ui_manager.h"
#include "../../hal/camera.h"
#include "../../hal/storage.h"
#include "../../core/app.h"

namespace ui {

static constexpr int SCREEN_W      = 240;
static constexpr int SCREEN_H      = 320;
static constexpr int TOP_BAR_H     = 40;
static constexpr int BOTTOM_BAR_H  = 70;
static constexpr int PREVIEW_AREA_H = SCREEN_H - TOP_BAR_H - BOTTOM_BAR_H;

// ---------------------------------------------------------------------------
// Button callbacks
// ---------------------------------------------------------------------------

/**
 * Retake: clear the PSRAM cache and return to the viewfinder.
 * The live camera feed resumes immediately since camera_task never stopped.
 */
static void on_retake(lv_event_t* e) {
    (void)e;
    camera_clear_cache();               // free captured_buf
    app_set_state(STATE_VIEWFINDER);    // back to main screen
}

/**
 * Save: write captured_buf to SD card as a timestamped BMP,
 * then clear the cache and return to the viewfinder.
 */
static void on_save(lv_event_t* e) {
    ScreenPreview* screen = static_cast<ScreenPreview*>(lv_event_get_user_data(e));
    if (screen) {
        screen->saveCurrentImage();
    }
}

// ---------------------------------------------------------------------------
// create
// ---------------------------------------------------------------------------
lv_obj_t* ScreenPreview::create() {
    root = lv_obj_create(NULL);
    lv_obj_add_style(root, &Theme::screen_bg, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    // No flex — all children are absolutely positioned so the canvas
    // can occupy the full 240x320 screen and bars sit on top as overlays.

    // ── Full-screen canvas (drawn first = behind the bars) ───────────────────
    canvas = lv_canvas_create(root);
    lv_obj_set_size(canvas, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(canvas, 0, 0);
    lv_obj_set_style_bg_color(canvas, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(canvas, LV_OPA_COVER, 0);

    // Attach captured_buf immediately if already available
    if (captured_buf && captured_w > 0 && captured_h > 0) {
        lv_canvas_set_buffer(canvas, captured_buf,
                             captured_w, captured_h,
                             LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_size(canvas, captured_w, captured_h);
        lv_obj_set_pos(canvas,
                       (SCREEN_W - captured_w) / 2,
                       (SCREEN_H - captured_h) / 2);
        lv_obj_invalidate(canvas);
    }

    // ── Top bar overlay ──────────────────────────────────────────────────────
    lv_obj_t* top = lv_obj_create(root);
    lv_obj_remove_style_all(top);
    lv_obj_add_style(top, &Theme::top_bar, 0);
    lv_obj_set_size(top, SCREEN_W, TOP_BAR_H);
    lv_obj_set_pos(top, 0, 0);
    create_label(top, &Theme::text_cursive, "Preview", LV_ALIGN_CENTER);

    // ── Status label (floating over the image) ───────────────────────────────
    status_label = lv_label_create(root);
    lv_obj_add_style(status_label, &Theme::text_body, 0);
    lv_label_set_text(status_label, "");
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);

    // ── Bottom bar overlay ───────────────────────────────────────────────────
    lv_obj_t* bottom = lv_obj_create(root);
    lv_obj_remove_style_all(bottom);
    lv_obj_add_style(bottom, &Theme::bottom_bar, 0);
    lv_obj_set_size(bottom, SCREEN_W, BOTTOM_BAR_H);
    lv_obj_set_pos(bottom, 0, SCREEN_H - BOTTOM_BAR_H);
    lv_obj_set_flex_flow(bottom, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom,
                          LV_FLEX_ALIGN_SPACE_AROUND,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    lv_obj_t* btn_retake = create_button(bottom, &Theme::btn_secondary,
                                         82, 40, LV_ALIGN_CENTER);
    create_label(btn_retake, &Theme::text_cursive, "Retake", LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_retake, on_retake, LV_EVENT_CLICKED, NULL);

    lv_obj_t* btn_save = create_button(bottom, &Theme::btn_primary,
                                       82, 40, LV_ALIGN_CENTER);
    create_label(btn_save, &Theme::text_cursive, "Save", LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_save, on_save, LV_EVENT_CLICKED, this);

    return root;
}

// ---------------------------------------------------------------------------
// destroy
// ---------------------------------------------------------------------------
void ScreenPreview::destroy() {
    if (root) {
        lv_obj_del(root);
        root         = nullptr;
        canvas       = nullptr;
        status_label = nullptr;
    }
    // NOTE: do NOT free captured_buf here — it is managed by camera_capture()
    // and camera_clear_cache(). The preview screen is just a viewer.
}

// ---------------------------------------------------------------------------
// showCapturedFrame
// Called by UIManager after navigating to this screen so the canvas displays
// whatever is currently in captured_buf (may be filled asynchronously by
// camera_task a few ms after we arrive here).
// ---------------------------------------------------------------------------
void ScreenPreview::showCapturedFrame() {
    if (!canvas) return;
    if (!captured_buf || captured_w <= 0 || captured_h <= 0) {
        setStatus("Waiting for capture...");
        return;
    }
    // Size canvas to exact image dimensions, center on the full 240x320 screen.
    lv_canvas_set_buffer(canvas, captured_buf,
                         captured_w, captured_h,
                         LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_size(canvas, captured_w, captured_h);
    lv_obj_set_pos(canvas,
                   (SCREEN_W - captured_w) / 2,
                   (SCREEN_H - captured_h) / 2);
    lv_obj_invalidate(canvas);
    setStatus("");
}

// ---------------------------------------------------------------------------
// saveCurrentImage — writes captured_buf to SD, then clears cache + navigates
// ---------------------------------------------------------------------------
bool ScreenPreview::saveCurrentImage() {
    if (!captured_buf || captured_w <= 0 || captured_h <= 0) {
        setStatus("Nothing to save");
        return false;
    }

    setStatus("Saving...");

    // Build a sequentially numbered filename
    char filename[40];
    snprintf(filename, sizeof(filename), "/photo_%lu.bmp", storage_get_next_photo_id());

    bool ok = storage_save_rgb565_bmp(
        reinterpret_cast<const uint8_t*>(captured_buf),
        captured_w,
        captured_h,
        filename);

    if (ok) {
        Serial.printf("[PREVIEW] Saved to SD: %s\n", filename);
        // Clear PSRAM cache and return to viewfinder
        camera_clear_cache();
        app_set_state(STATE_VIEWFINDER);
    } else {
        setStatus("Save FAILED — check SD card");
    }
    return ok;
}

// ---------------------------------------------------------------------------
// setStatus
// ---------------------------------------------------------------------------
void ScreenPreview::setStatus(const char* text) {
    if (status_label) {
        lv_label_set_text(status_label, text);
    }
    Serial.printf("[PREVIEW] %s\n", text);
}

} // namespace ui
