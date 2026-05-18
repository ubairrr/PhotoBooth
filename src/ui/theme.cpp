#include "theme.h"

LV_FONT_DECLARE(pacifico);

namespace ui {

const lv_color_t Colors::PRIMARY = lv_color_hex(0xFF4FA3);
const lv_color_t Colors::PRIMARY_DARK = lv_color_hex(0xE63E91);
const lv_color_t Colors::LIGHT = lv_color_hex(0xFFC1DA);
const lv_color_t Colors::BG = lv_color_hex(0xFFF0F6);
const lv_color_t Colors::TEXT = lv_color_hex(0x4A2C3A);
const lv_color_t Colors::WHITE = lv_color_hex(0xFFFFFF);

// ===== Static style objects =====
lv_style_t Theme::screen_bg;
lv_style_t Theme::top_bar;
lv_style_t Theme::bottom_bar;
lv_style_t Theme::card;

lv_style_t Theme::btn_primary;
lv_style_t Theme::btn_secondary;
lv_style_t Theme::btn_capture;

lv_style_t Theme::text_title;
lv_style_t Theme::text_body;
lv_style_t Theme::text_cursive;

lv_style_t Theme::focus_frame;

// ===== Init =====
void Theme::init() {

    // -------- Screen Background --------
    lv_style_init(&screen_bg);
    lv_style_set_bg_color(&screen_bg, Colors::BG);
    lv_style_set_bg_opa(&screen_bg, LV_OPA_COVER);

    // -------- Top Bar --------
    lv_style_init(&top_bar);
    lv_style_set_bg_color(&top_bar, Colors::LIGHT);
    lv_style_set_bg_opa(&top_bar, LV_OPA_80);
    lv_style_set_radius(&top_bar, 0);
    lv_style_set_pad_all(&top_bar, 8);

    // -------- Bottom Bar --------
    lv_style_init(&bottom_bar);
    lv_style_set_bg_color(&bottom_bar, Colors::LIGHT);
    lv_style_set_bg_opa(&bottom_bar, LV_OPA_90);
    lv_style_set_radius(&bottom_bar, 12);
    lv_style_set_pad_all(&bottom_bar, 8);

    // -------- Card --------
    lv_style_init(&card);
    lv_style_set_bg_color(&card, Colors::WHITE);
    lv_style_set_bg_opa(&card, LV_OPA_90);
    lv_style_set_radius(&card, 12);
    lv_style_set_pad_all(&card, 8);

    // Keep shadows VERY light (performance)
    lv_style_set_shadow_width(&card, 6);
    lv_style_set_shadow_opa(&card, LV_OPA_20);
    lv_style_set_shadow_color(&card, Colors::PRIMARY);

    // -------- Primary Button --------
    lv_style_init(&btn_primary);
    lv_style_set_bg_color(&btn_primary, Colors::PRIMARY);
    lv_style_set_bg_opa(&btn_primary, LV_OPA_COVER);
    lv_style_set_radius(&btn_primary, 20);
    lv_style_set_pad_all(&btn_primary, 10);

    lv_style_set_text_color(&btn_primary, Colors::WHITE);

    // -------- Secondary Button --------
    lv_style_init(&btn_secondary);
    lv_style_set_bg_color(&btn_secondary, Colors::WHITE);
    lv_style_set_bg_opa(&btn_secondary, LV_OPA_90);
    lv_style_set_radius(&btn_secondary, 20);
    lv_style_set_border_width(&btn_secondary, 2);
    lv_style_set_border_color(&btn_secondary, Colors::PRIMARY);
    lv_style_set_text_color(&btn_secondary, Colors::PRIMARY);

    // -------- Capture Button --------
    lv_style_init(&btn_capture);
    lv_style_set_bg_color(&btn_capture, Colors::PRIMARY);
    lv_style_set_radius(&btn_capture, LV_RADIUS_CIRCLE);
    lv_style_set_width(&btn_capture, 62);
    lv_style_set_height(&btn_capture, 62);

    // subtle glow (cheap)
    lv_style_set_shadow_width(&btn_capture, 10);
    lv_style_set_shadow_opa(&btn_capture, LV_OPA_30);
    lv_style_set_shadow_color(&btn_capture, Colors::PRIMARY);

    // -------- Title Text --------
    lv_style_init(&text_title);
    lv_style_set_text_color(&text_title, Colors::TEXT);
    lv_style_set_text_font(&text_title, &lv_font_montserrat_18);

    // -------- Body Text --------
    lv_style_init(&text_body);
    lv_style_set_text_color(&text_body, Colors::TEXT);
    lv_style_set_text_font(&text_body, &lv_font_montserrat_14);

    // -------- Cursive Text --------
    lv_style_init(&text_cursive);
    lv_style_set_text_color(&text_cursive, lv_color_hex(0xE84A78));
    lv_style_set_text_font(&text_cursive, &pacifico);

    // -------- Focus Frame --------
    lv_style_init(&focus_frame);
    lv_style_set_border_color(&focus_frame, Colors::WHITE);
    lv_style_set_border_width(&focus_frame, 2);
    lv_style_set_border_opa(&focus_frame, LV_OPA_80);
    lv_style_set_radius(&focus_frame, 8);
    lv_style_set_bg_opa(&focus_frame, LV_OPA_TRANSP);
}

} // namespace ui
