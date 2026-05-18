// =============================================================================
// screen_share.cpp  —  WiFi sharing active screen
//
// Layout (240×320 portrait):
//   ┌─────────────────────────┐
//   │   📡  Sharing Gallery   │  top_bar
//   ├─────────────────────────┤
//   │                         │
//   │   ┌─────────────────┐   │
//   │   │   QR CODE       │   │  160×160 — links to http://192.168.4.1
//   │   └─────────────────┘   │
//   │                         │
//   │  1. Connect to WiFi:    │
//   │     Maryam's Gallery    │
//   │     Pass: photobooth    │
//   │                         │
//   │  2. Scan QR to open     │
//   │     gallery in browser  │
//   │                         │
//   ├─────────────────────────┤
//   │     [ Stop Sharing ]    │  btn_primary
//   └─────────────────────────┘
// =============================================================================
#include "screen_share.h"

#include "theme.h"
#include "ui_helpers.h"
#include "ui_manager.h"
#include "../../core/app.h"
#include "../../hal/wifi_share.h"
#include "../assets.h"

namespace ui {

static constexpr int SCREEN_W = 240;
static constexpr int SCREEN_H = 320;
static constexpr int HEADER_H = 40;
static constexpr int FOOTER_H = 46;

lv_obj_t* ScreenShare::create() {
    root = lv_obj_create(NULL);
    lv_obj_add_style(root, &Theme::screen_bg, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(root, SCREEN_W, SCREEN_H);

    // ── Header bar ────────────────────────────────────────────────────────
    lv_obj_t* hdr = lv_obj_create(root);
    lv_obj_remove_style_all(hdr);
    lv_obj_add_style(hdr, &Theme::top_bar, 0);
    lv_obj_set_size(hdr, SCREEN_W, HEADER_H);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* hdr_lbl = lv_label_create(hdr);
    lv_obj_add_style(hdr_lbl, &Theme::text_cursive, 0);
    lv_label_set_text(hdr_lbl, "Sharing Gallery");

    // ── Content area (between header and footer) ──────────────────────────
    const int content_y = HEADER_H + 2; // Reduced top margin

    // QR code image — centred horizontally
    lv_obj_t* qr_img = lv_img_create(root);
    lv_img_set_src(qr_img, &img_qr_code);
    lv_obj_set_pos(qr_img, (SCREEN_W - 160) / 2, content_y);

    // Instruction block below QR
    const int text_y = content_y + 160 + 2; // Reduced gap below QR

    // Step 1 — WiFi name & Pass
    lv_obj_t* lbl1 = lv_label_create(root);
    lv_obj_add_style(lbl1, &Theme::text_body, 0);
    lv_label_set_text_fmt(lbl1, "1. WiFi: %s", SHARE_SSID);
    lv_obj_set_pos(lbl1, 14, text_y);
    lv_obj_set_style_text_color(lbl1, Colors::PRIMARY, 0);

    lv_obj_t* lbl_pass = lv_label_create(root);
    lv_obj_add_style(lbl_pass, &Theme::text_body, 0);
    lv_label_set_text_fmt(lbl_pass, "   Pass: %s", SHARE_PASSWORD);
    lv_obj_set_pos(lbl_pass, 14, text_y + 18);

    // Step 2 — Scan QR
    lv_obj_t* lbl2 = lv_label_create(root);
    lv_obj_add_style(lbl2, &Theme::text_body, 0);
    lv_label_set_text(lbl2, "2. Scan QR to view photos");
    lv_obj_set_pos(lbl2, 14, text_y + 36); // Ends at 36 + 14 = 50. text_y is 208, so ends at 258. safely above 264!
    lv_obj_set_style_text_opa(lbl2, LV_OPA_70, 0);

    // ── Footer — Stop Sharing button ──────────────────────────────────────
    lv_obj_t* footer = lv_obj_create(root);
    lv_obj_remove_style_all(footer);
    lv_obj_add_style(footer, &Theme::bottom_bar, 0);
    lv_obj_set_size(footer, SCREEN_W, FOOTER_H);
    lv_obj_set_pos(footer, 0, SCREEN_H - FOOTER_H);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* btn_stop = create_button(footer, &Theme::btn_primary,
                                       200, 40, LV_ALIGN_CENTER);
    create_label(btn_stop, &Theme::text_body, "Stop Sharing", LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_stop, [](lv_event_t*) {
        wifi_share_stop();
        app_set_state(STATE_GALLERY);
    }, LV_EVENT_CLICKED, nullptr);

    return root;
}

void ScreenShare::destroy() {
    if (!root) return;
    lv_obj_del(root);
    root = nullptr;
}

} // namespace ui
