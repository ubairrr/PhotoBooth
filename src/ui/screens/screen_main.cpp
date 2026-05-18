#include "screen_main.h"
#include "../../core/app.h"
#include "../../hal/camera.h"
#include "../assets.h"
#include "theme.h"
#include "ui_helpers.h"
#include "ui_manager.h"

namespace ui {

static constexpr int SCREEN_W = 240;
static constexpr int SCREEN_H = 320;
static constexpr int TOP_BAR_H = 40;
static constexpr int BOTTOM_BAR_H = 68;

static void on_filters_click(lv_event_t *e) {
  (void)e;
  app_set_state(STATE_FILTERS);
}

static void on_gallery_click(lv_event_t *e) {
  (void)e;
  app_set_state(STATE_GALLERY);
}

static void on_settings_click(lv_event_t *e) {
  (void)e;
  app_set_state(STATE_SETTINGS);
}

static void on_capture_click(lv_event_t *e) {
  (void)e;
  UIManager::get().mainScreen.startCountdown();
}

lv_obj_t *ScreenMain::create() {
  root = lv_obj_create(NULL);
  lv_obj_add_style(root, &Theme::screen_bg, 0);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

  buildCameraArea();
  buildTopBar();
  buildBottomBar();

  return root;
}

void ScreenMain::destroy() {
  if (root) {
    lv_obj_del(root);
    root = nullptr;
    top_bar = nullptr;
    camera_container = nullptr;
    canvas = nullptr;
    bottom_bar = nullptr;
    btn_gallery = nullptr;
    if (countdown_timer) {
      lv_timer_del(countdown_timer);
      countdown_timer = nullptr;
    }
    timer_label = nullptr;
    countdown_label = nullptr;
  }
}

void ScreenMain::updateCanvas() {
  if (canvas) {
    lv_obj_invalidate(canvas);
  }
}

void ScreenMain::setCanvasBuffer(uint8_t *buffer, int w, int h) {
  canvasBuffer = buffer;
  canvasWidth = w;
  canvasHeight = h;

  if (!canvas)
    return;

  lv_canvas_set_buffer(canvas, buffer, w, h, LV_IMG_CF_TRUE_COLOR);
  // Center the camera image in the full screen
  lv_obj_set_pos(canvas, (SCREEN_W - w) / 2, (SCREEN_H - h) / 2);
  lv_obj_invalidate(canvas);
}

void ScreenMain::openPreview() {
  // openPreview() is no longer used — capture goes through camera_capture().
  // Kept as a no-op to avoid linker errors in case of any stale references.
}

void ScreenMain::startCountdown() {
  if (countdown_timer)
    return; // Already counting

  if (app_timer_duration == 0) {
    // Immediate capture
    if (app_photobooth_style_capture) {
      lv_refr_now(NULL);

      lv_img_dsc_t* snap = lv_snapshot_take(root, LV_IMG_CF_TRUE_COLOR);
      if (snap) {
        if (snap->data) {
          if (!captured_buf) {
            captured_buf = (lv_color_t*)ps_malloc(240 * 320 * sizeof(lv_color_t));
          }
          if (captured_buf) {
            memcpy(captured_buf, snap->data, 240 * 320 * sizeof(lv_color_t));
            captured_w = 240;
            captured_h = 320;
          }
        }
        lv_snapshot_free(snap);
      }
      capture_filtered = true;
    } else {
      camera_capture();
    }
    app_set_state(STATE_PREVIEW);
    return;
  }

  countdown_value = app_timer_duration;
  if (timer_label) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%ds", countdown_value);
    lv_label_set_text(timer_label, buf);
  }
  
  if (countdown_label) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", countdown_value);
    lv_label_set_text(countdown_label, buf);
    lv_obj_clear_flag(countdown_label, LV_OBJ_FLAG_HIDDEN);
  }

  countdown_timer = lv_timer_create(countdown_timer_cb, 1000, this);
}

void ScreenMain::countdown_timer_cb(lv_timer_t *t) {
  ScreenMain *self = (ScreenMain *)t->user_data;
  self->countdown_value--;

  if (self->countdown_value <= 0) {
    if (self->timer_label)
      lv_label_set_text(self->timer_label, "");
    if (self->countdown_label)
      lv_obj_add_flag(self->countdown_label, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(t);
    self->countdown_timer = nullptr;

    // Perform capture
    if (app_photobooth_style_capture) {
      // Force screen redraw to clear timer labels before snapshot
      lv_refr_now(NULL);

      lv_img_dsc_t* snap = lv_snapshot_take(self->root, LV_IMG_CF_TRUE_COLOR);
      if (snap) {
        if (snap->data) {
          // Ensure we have a cache buffer allocated
          if (!captured_buf) {
            captured_buf = (lv_color_t*)ps_malloc(240 * 320 * sizeof(lv_color_t));
          }
          if (captured_buf) {
            memcpy(captured_buf, snap->data, 240 * 320 * sizeof(lv_color_t));
            captured_w = 240;
            captured_h = 320;
          }
        }
        lv_snapshot_free(snap);
      }
      capture_filtered = true; // Image already has filter + UI applied
    } else {
      camera_capture();
    }
    
    app_set_state(STATE_PREVIEW);
  } else {
    if (self->timer_label) {
      char buf[8];
      snprintf(buf, sizeof(buf), "%ds", self->countdown_value);
      lv_label_set_text(self->timer_label, buf);
    }
    if (self->countdown_label) {
      char buf[8];
      snprintf(buf, sizeof(buf), "%d", self->countdown_value);
      lv_label_set_text(self->countdown_label, buf);
    }
  }
}

//////////////////////////////////////////////////////////////
// TOP BAR
//////////////////////////////////////////////////////////////
void ScreenMain::buildTopBar() {
  top_bar = lv_obj_create(root);
  lv_obj_remove_style_all(top_bar);
  lv_obj_add_style(top_bar, &Theme::top_bar, 0);

  lv_obj_set_size(top_bar, SCREEN_W, TOP_BAR_H);
  lv_obj_set_pos(top_bar, 0, 0);
  lv_obj_set_flex_flow(top_bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(top_bar, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  create_label(top_bar, &Theme::text_cursive, "Maryam's Booth",
               LV_ALIGN_LEFT_MID, 28, 0);

  btn_gallery = lv_btn_create(top_bar);
  lv_obj_add_style(btn_gallery, &Theme::btn_secondary, 0);
  lv_obj_set_size(btn_gallery, 32, 32);
  create_label(btn_gallery, &Theme::text_body, LV_SYMBOL_IMAGE,
               LV_ALIGN_CENTER);
  lv_obj_add_event_cb(btn_gallery, on_gallery_click, LV_EVENT_CLICKED, NULL);

  top_cat.create(top_bar, &cat_peek, -4, 8);
  top_cat.startFloatAnim(2, 2200);

  top_bow.create(top_bar, &bow_small, 145, -11);
  top_bow.setAngle(250); // +25 degrees (tilt towards +x)
  top_bow.startFloatAnim(1, 1800);
}

//////////////////////////////////////////////////////////////
// CAMERA AREA
//////////////////////////////////////////////////////////////
void ScreenMain::buildCameraArea() {
  // Full screen viewport
  camera_container = lv_obj_create(root);
  lv_obj_remove_style_all(camera_container);
  lv_obj_set_size(camera_container, SCREEN_W, SCREEN_H);
  lv_obj_set_pos(camera_container, 0, 0);
  lv_obj_clear_flag(camera_container, LV_OBJ_FLAG_SCROLLABLE);

  // Canvas — fills the container
  canvas = lv_canvas_create(camera_container);
  lv_obj_remove_style_all(canvas);
  lv_obj_set_size(canvas, SCREEN_W, SCREEN_H);
  lv_obj_set_pos(canvas, 0, 0);
  lv_obj_set_style_bg_color(canvas, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(canvas, LV_OPA_COVER, 0);
  lv_obj_set_style_border_opa(canvas, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_all(canvas, 0, 0);

  if (canvasBuffer) {
    lv_canvas_set_buffer(canvas, canvasBuffer, canvasWidth, canvasHeight,
                         LV_IMG_CF_TRUE_COLOR);
    // Center in full screen
    lv_obj_set_pos(canvas, (SCREEN_W - canvasWidth) / 2,
                   (SCREEN_H - canvasHeight) / 2);
  }

  // Focus frame
  lv_obj_t *frame = lv_obj_create(camera_container);
  lv_obj_remove_style_all(frame);
  lv_obj_add_style(frame, &Theme::focus_frame, 0);
  lv_obj_set_size(frame, 180, 200);
  lv_obj_align(frame, LV_ALIGN_CENTER, 0, 0);

  // Timer label
  timer_label = create_label(camera_container, &Theme::text_title, "",
                                 LV_ALIGN_TOP_RIGHT, -12, 12);

  // Large Countdown Overlay (Center)
  countdown_label = lv_label_create(camera_container);
  lv_obj_add_style(countdown_label, &Theme::text_cursive, 0);
  lv_obj_set_style_text_color(countdown_label, Colors::PRIMARY, 0);
  lv_obj_align(countdown_label, LV_ALIGN_CENTER, 0, -20);
  lv_obj_add_flag(countdown_label, LV_OBJ_FLAG_HIDDEN);
}

//////////////////////////////////////////////////////////////
// BOTTOM BAR
//////////////////////////////////////////////////////////////
void ScreenMain::buildBottomBar() {
  bottom_bar = lv_obj_create(root);
  lv_obj_remove_style_all(bottom_bar);
  lv_obj_add_style(bottom_bar, &Theme::bottom_bar, 0);

  lv_obj_set_size(bottom_bar, SCREEN_W, BOTTOM_BAR_H);
  lv_obj_set_pos(bottom_bar, 0, SCREEN_H - BOTTOM_BAR_H);

  lv_obj_set_flex_flow(bottom_bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bottom_bar, LV_FLEX_ALIGN_SPACE_AROUND,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Filters button
  btn_filters =
      create_button(bottom_bar, &Theme::btn_secondary, 58, 38, LV_ALIGN_CENTER);

  create_label(btn_filters, &Theme::text_body, "FX", LV_ALIGN_CENTER);

  lv_obj_add_event_cb(btn_filters, on_filters_click, LV_EVENT_CLICKED, NULL);

  // Capture button (custom layered)
  btn_capture = createCaptureButton(bottom_bar);
  lv_obj_add_event_cb(btn_capture, on_capture_click, LV_EVENT_CLICKED, NULL);

  // Settings button (right of capture button)
  lv_obj_t *btn_settings =
      create_button(bottom_bar, &Theme::btn_secondary, 58, 38, LV_ALIGN_CENTER);
  create_label(btn_settings, &Theme::text_body, LV_SYMBOL_SETTINGS,
               LV_ALIGN_CENTER);
  lv_obj_add_event_cb(btn_settings, on_settings_click, LV_EVENT_CLICKED, NULL);

  left_cat.create(bottom_bar, &cat_idle, -4, -8);
  left_cat.startFloatAnim(3, 2500);

  right_cat.create(bottom_bar, &cat_happy, 135, -8);
  right_cat.startFloatAnim(2, 2000);
}

//////////////////////////////////////////////////////////////
// CAPTURE BUTTON (Layered Design)
//////////////////////////////////////////////////////////////
lv_obj_t *ScreenMain::createCaptureButton(lv_obj_t *parent) {

  // Outer button
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_add_style(btn, &Theme::btn_capture, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

  // Inner ring (white circle)
  lv_obj_t *ring = lv_obj_create(btn);
  lv_obj_remove_style_all(ring);
  lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_size(ring, 44, 44);
  lv_obj_center(ring);

  lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(ring, Colors::WHITE, 0);
  lv_obj_set_style_bg_opa(ring, LV_OPA_COVER, 0);

  // Inner core (pink)
  lv_obj_t *core = lv_obj_create(ring);
  lv_obj_remove_style_all(core);
  lv_obj_clear_flag(core, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(core, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_size(core, 30, 30);
  lv_obj_center(core);

  lv_obj_set_style_radius(core, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(core, Colors::PRIMARY, 0);

  // Flower icon inside core
  lv_obj_t* flower = lv_img_create(core);
  lv_img_set_src(flower, &flower_small);
  lv_obj_center(flower);

  return btn;
}

} // namespace ui
