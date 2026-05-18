// =============================================================================
// screen_gallery.cpp  —  SD-card photo browser
//
// VIEW_GRID  : scrollable thumbnail grid.  Scans / on SD_MMC for *.bmp files.
//              Thumbnails are 4× downsampled into PSRAM-allocated descriptors.
//
// VIEW_DETAIL: fullscreen viewer for a selected photo (BMP decoded into PSRAM,
//              bound to an lv_canvas) with a Back and a Delete button.
//
// Both old 320×240 (landscape) and new 240×320 (portrait) BMPs are handled.
// =============================================================================
#include "screen_gallery.h"

#include <Arduino.h>
#include <SD_MMC.h>
#include <algorithm>

#include "theme.h"
#include "ui_helpers.h"
#include "ui_manager.h"
#include "../../core/app.h"
#include "../../hal/storage.h"
#include "../../hal/wifi_share.h"

namespace ui {

// ── Layout constants ─────────────────────────────────────────────────────────
static constexpr int SCREEN_W  = 240;
static constexpr int SCREEN_H  = 320;
static constexpr int HEADER_H  = 44;
static constexpr int FOOTER_H  = 56;   // detail view bottom bar
static constexpr int THUMB_SCALE = 4;  // divisor applied to BMP dimensions

// ─────────────────────────────────────────────────────────────────────────────
//  BMP utilities (file-scope, no class state)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Parse the BMP DIB header and return image width/height.
 * header[] must be at least 54 bytes (standard BMP file+info header).
 */
static void bmp_dims(const uint8_t* hdr, int* w, int* h) {
    *w = hdr[18] | (hdr[19] << 8) | (hdr[20] << 16) | (hdr[21] << 24);
    *h = hdr[22] | (hdr[23] << 8) | (hdr[24] << 16) | (hdr[25] << 24);
}

/** BMP row stride: 24-bit pixels, padded to 4-byte boundary. */
static int bmp_stride(int w) { return (w * 3 + 3) & ~3; }

/**
 * loadBmpThumbnail  —  4× downsampled LVGL image descriptor.
 *
 * Works for any BMP width/height up to 1024.  Handles both 320×240 (old
 * landscape) and 240×320 (new portrait) saves.
 *
 * Memory layout: single ps_malloc block
 *   [ lv_img_dsc_t | pixel data (RGB565 LE) ]
 * Caller must free() the returned pointer when done.
 */
lv_img_dsc_t* ScreenGallery::loadBmpThumbnail(const char* path,
                                               int* out_w, int* out_h) {
    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
        Serial.printf("[GALLERY] Cannot open: %s\n", path);
        return nullptr;
    }

    uint8_t hdr[54];
    if (f.read(hdr, 54) != 54 || hdr[0] != 'B' || hdr[1] != 'M') {
        Serial.printf("[GALLERY] Not a BMP: %s\n", path);
        f.close();
        return nullptr;
    }

    int orig_w, orig_h;
    bmp_dims(hdr, &orig_w, &orig_h);

    if (orig_w <= 0 || orig_w > 1024 || orig_h <= 0 || orig_h > 1024) {
        Serial.printf("[GALLERY] Bad dims %dx%d: %s\n", orig_w, orig_h, path);
        f.close();
        return nullptr;
    }

    if (out_w) *out_w = orig_w;
    if (out_h) *out_h = orig_h;

    const int tw = orig_w / THUMB_SCALE;
    const int th = orig_h / THUMB_SCALE;
    const int stride = bmp_stride(orig_w);

    size_t data_sz = (size_t)tw * th * 2;   // RGB565, 2 bytes/pixel
    uint8_t* block = (uint8_t*)ps_malloc(sizeof(lv_img_dsc_t) + data_sz);
    if (!block) {
        Serial.printf("[GALLERY] ps_malloc failed for thumb: %s\n", path);
        f.close();
        return nullptr;
    }

    lv_img_dsc_t* dsc = (lv_img_dsc_t*)block;
    uint8_t* px       = block + sizeof(lv_img_dsc_t);

    dsc->header.always_zero = 0;
    dsc->header.cf          = LV_IMG_CF_TRUE_COLOR;
    dsc->header.w           = (uint32_t)tw;
    dsc->header.h           = (uint32_t)th;
    dsc->data_size          = data_sz;
    dsc->data               = px;

    // Row buffer on heap (stride can be up to 960+ bytes — avoid stack overflow)
    uint8_t* row = (uint8_t*)malloc(stride);
    if (!row) {
        free(block);
        f.close();
        return nullptr;
    }

    // BMP rows are stored bottom-up.
    for (int y = orig_h - 1; y >= 0; y--) {
        int got = f.read(row, stride);
        if (got != stride) {
            Serial.printf("[GALLERY] Short read row %d (%d/%d) in %s\n",
                          y, got, stride, path);
            break;
        }
        // Sample every THUMB_SCALE-th row
        if (y % THUMB_SCALE == 0) {
            int ty = y / THUMB_SCALE;
            for (int tx = 0; tx < tw; tx++) {
                int ox = tx * THUMB_SCALE;
                uint8_t b = row[ox * 3 + 0];   // BMP = BGR order
                uint8_t g = row[ox * 3 + 1];
                uint8_t r = row[ox * 3 + 2];
                // BGR888 → RGB565, little-endian (LVGL TRUE_COLOR)
                uint16_t c = ((uint16_t)(r & 0xF8) << 8)
                           | ((uint16_t)(g & 0xFC) << 3)
                           | (b >> 3);
                px[(ty * tw + tx) * 2 + 0] = c & 0xFF;
                px[(ty * tw + tx) * 2 + 1] = (c >> 8) & 0xFF;
            }
        }
        if ((y & 0x1F) == 0) yield();   // watchdog every 32 rows
    }

    free(row);
    f.close();
    Serial.printf("[GALLERY] Thumb OK  %s  src:%dx%d  th:%dx%d\n",
                  path, orig_w, orig_h, tw, th);
    return dsc;
}

/**
 * loadBmpFull  —  decode an entire BMP into a ps_malloc'd RGB565 buffer.
 *
 * Handles both portrait and landscape files.
 * Caller must free() the returned buffer when done.
 */
uint8_t* ScreenGallery::loadBmpFull(const char* path, int* out_w, int* out_h) {
    File f = SD_MMC.open(path, FILE_READ);
    if (!f) return nullptr;

    uint8_t hdr[54];
    if (f.read(hdr, 54) != 54 || hdr[0] != 'B' || hdr[1] != 'M') {
        f.close();
        return nullptr;
    }

    int orig_w, orig_h;
    bmp_dims(hdr, &orig_w, &orig_h);

    if (orig_w <= 0 || orig_w > 1024 || orig_h <= 0 || orig_h > 1024) {
        f.close();
        return nullptr;
    }

    *out_w = orig_w;
    *out_h = orig_h;

    const int stride = bmp_stride(orig_w);
    size_t px_sz = (size_t)orig_w * orig_h * 2;  // RGB565

    uint8_t* pxbuf = (uint8_t*)ps_malloc(px_sz);
    if (!pxbuf) {
        Serial.printf("[GALLERY] ps_malloc failed for full BMP: %s\n", path);
        f.close();
        return nullptr;
    }

    uint8_t* row = (uint8_t*)malloc(stride);
    if (!row) { free(pxbuf); f.close(); return nullptr; }

    // BMP rows are bottom-up → write into pxbuf top-down
    for (int y = orig_h - 1; y >= 0; y--) {
        int got = f.read(row, stride);
        if (got != stride) {
            Serial.printf("[GALLERY] Short read row %d in %s\n", y, path);
            break;
        }
        // Destination row (since LVGL buffer is top-down, and BMP is bottom-up,
        // the first row we read from file is y = orig_h - 1. We want it at the
        // bottom of our buffer, so dst_row = y)
        int dst_row = y;
        for (int x = 0; x < orig_w; x++) {
            uint8_t b = row[x * 3 + 0];
            uint8_t g = row[x * 3 + 1];
            uint8_t r = row[x * 3 + 2];
            uint16_t c = ((uint16_t)(r & 0xF8) << 8)
                       | ((uint16_t)(g & 0xFC) << 3)
                       | (b >> 3);
            pxbuf[(dst_row * orig_w + x) * 2 + 0] = c & 0xFF;
            pxbuf[(dst_row * orig_w + x) * 2 + 1] = (c >> 8) & 0xFF;
        }
        if ((y & 0xF) == 0) yield();
    }

    free(row);
    f.close();
    Serial.printf("[GALLERY] Full BMP loaded: %s  %dx%d\n", path, orig_w, orig_h);
    return pxbuf;
}

// ─────────────────────────────────────────────────────────────────────────────
//  create / destroy
// ─────────────────────────────────────────────────────────────────────────────

lv_obj_t* ScreenGallery::create() {
    root = lv_obj_create(NULL);
    lv_obj_add_style(root, &Theme::screen_bg, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(root, SCREEN_W, SCREEN_H);

    buildGridView();
    buildDetailView();
    buildHeader();    // built last so it draws on top of both views

    loadFromSD();
    showGrid();

    return root;
}

void ScreenGallery::destroy() {
    if (!root) return;

    lv_obj_del(root);
    root          = nullptr;
    grid_view     = nullptr;
    detail_view   = nullptr;
    detail_canvas = nullptr;
    detail_status = nullptr;

    // Free all thumbnail descriptors (pixel data is in the same ps_malloc block)
    for (auto& e : entries) {
        if (e.thumb) { free(e.thumb); e.thumb = nullptr; }
    }
    entries.clear();
    selected_idx = -1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Header  (shared by both views — only the title changes)
// ─────────────────────────────────────────────────────────────────────────────

void ScreenGallery::buildHeader() {
    lv_obj_t* hdr = lv_obj_create(root);
    lv_obj_remove_style_all(hdr);
    lv_obj_add_style(hdr, &Theme::top_bar, 0);
    lv_obj_set_size(hdr, SCREEN_W, HEADER_H);
    lv_obj_set_pos(hdr, 0, 0);

    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(hdr, 10, 0);
    lv_obj_set_style_pad_right(hdr, 10, 0);

    create_label(hdr, &Theme::text_cursive, "Gallery", LV_ALIGN_LEFT_MID);

    // ── Share button (WiFi AP gallery) ──────────────────────────────────
    lv_obj_t* btn_share = create_button(hdr, &Theme::btn_primary,
                                        60, 36, LV_ALIGN_RIGHT_MID);
    create_label(btn_share, &Theme::text_body, "Share", LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_share, [](lv_event_t*) {
        wifi_share_start();
        app_set_state(STATE_SHARE);
    }, LV_EVENT_CLICKED, nullptr);

    // ── Back button ──────────────────────────────────────────────────────
    lv_obj_t* btn_back = create_button(hdr, &Theme::btn_secondary,
                                       40, 36, LV_ALIGN_RIGHT_MID);
    create_label(btn_back, &Theme::text_body, "<", LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_back, [](lv_event_t*) {
        app_set_state(STATE_VIEWFINDER);
    }, LV_EVENT_CLICKED, nullptr);
}


// ─────────────────────────────────────────────────────────────────────────────
//  Grid view
// ─────────────────────────────────────────────────────────────────────────────

void ScreenGallery::buildGridView() {
    const int grid_footer_h = 40;
    grid_view = lv_obj_create(root);
    lv_obj_remove_style_all(grid_view);
    lv_obj_set_size(grid_view, SCREEN_W, SCREEN_H - HEADER_H - grid_footer_h);
    lv_obj_set_pos(grid_view, 0, HEADER_H);

    lv_obj_set_flex_flow(grid_view, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid_view,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(grid_view, 6, 0);
    lv_obj_set_style_pad_row(grid_view, 6, 0);
    lv_obj_set_style_pad_column(grid_view, 6, 0);
    lv_obj_set_scroll_dir(grid_view, LV_DIR_VER);
    lv_obj_set_style_bg_opa(grid_view, LV_OPA_TRANSP, 0);

    // ── Pagination Footer ────────────────────────────────────────────────────
    grid_footer = lv_obj_create(root);
    lv_obj_remove_style_all(grid_footer);
    lv_obj_add_style(grid_footer, &Theme::bottom_bar, 0);
    lv_obj_set_size(grid_footer, SCREEN_W, grid_footer_h);
    lv_obj_set_pos(grid_footer, 0, SCREEN_H - grid_footer_h);
    lv_obj_set_flex_flow(grid_footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(grid_footer,
                          LV_FLEX_ALIGN_SPACE_AROUND,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    btn_prev = create_button(grid_footer, &Theme::btn_secondary, 50, 32, LV_ALIGN_CENTER);
    create_label(btn_prev, &Theme::text_body, "<", LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_prev, [](lv_event_t* e) {
        ScreenGallery* self = static_cast<ScreenGallery*>(lv_event_get_user_data(e));
        if (self->current_page > 0) {
            self->current_page--;
            self->loadPage(self->current_page);
        }
    }, LV_EVENT_CLICKED, this);

    page_label = create_label(grid_footer, &Theme::text_body, "1 / 1", LV_ALIGN_CENTER);

    btn_next = create_button(grid_footer, &Theme::btn_secondary, 50, 32, LV_ALIGN_CENTER);
    create_label(btn_next, &Theme::text_body, ">", LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_next, [](lv_event_t* e) {
        ScreenGallery* self = static_cast<ScreenGallery*>(lv_event_get_user_data(e));
        int total_pages = (self->all_photos.size() + PAGE_SIZE - 1) / PAGE_SIZE;
        if (self->current_page < total_pages - 1) {
            self->current_page++;
            self->loadPage(self->current_page);
        }
    }, LV_EVENT_CLICKED, this);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Detail view  (hidden until a thumbnail is tapped)
// ─────────────────────────────────────────────────────────────────────────────

void ScreenGallery::buildDetailView() {
    // Full-screen container — the header (built last) and the footer bar
    // inside this view both float as overlays over the image canvas.
    detail_view = lv_obj_create(root);
    lv_obj_remove_style_all(detail_view);
    lv_obj_add_style(detail_view, &Theme::screen_bg, 0);
    lv_obj_set_size(detail_view, SCREEN_W, SCREEN_H);   // full screen
    lv_obj_set_pos(detail_view, 0, 0);
    lv_obj_clear_flag(detail_view, LV_OBJ_FLAG_SCROLLABLE);

    // Canvas — sized to buffer in showDetail(); black placeholder for now
    detail_canvas = lv_canvas_create(detail_view);
    lv_obj_set_size(detail_canvas, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(detail_canvas, 0, 0);
    lv_obj_set_style_bg_color(detail_canvas, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(detail_canvas, LV_OPA_COVER, 0);

    // Status label (shown while loading)
    detail_status = lv_label_create(detail_view);
    lv_obj_add_style(detail_status, &Theme::text_body, 0);
    lv_label_set_text(detail_status, "");
    lv_obj_align(detail_status, LV_ALIGN_CENTER, 0, 0);

    // Footer bar — floats at the bottom of the full-screen view
    lv_obj_t* bar = lv_obj_create(detail_view);
    lv_obj_remove_style_all(bar);
    lv_obj_add_style(bar, &Theme::bottom_bar, 0);
    lv_obj_set_size(bar, SCREEN_W, FOOTER_H);
    lv_obj_set_pos(bar, 0, SCREEN_H - FOOTER_H);   // stick to bottom
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar,
                          LV_FLEX_ALIGN_SPACE_AROUND,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    // ← Back to grid
    lv_obj_t* btn_back = create_button(bar, &Theme::btn_secondary,
                                       90, 38, LV_ALIGN_CENTER);
    create_label(btn_back, &Theme::text_body, "Back", LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_back, [](lv_event_t* e) {
        ScreenGallery* self = static_cast<ScreenGallery*>(lv_event_get_user_data(e));
        self->showGrid();
    }, LV_EVENT_CLICKED, this);

    // 🗑 Delete
    lv_obj_t* btn_del = create_button(bar, &Theme::btn_primary,
                                      90, 38, LV_ALIGN_CENTER);
    create_label(btn_del, &Theme::text_body, "Delete", LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_del, [](lv_event_t* e) {
        ScreenGallery* self = static_cast<ScreenGallery*>(lv_event_get_user_data(e));
        self->deleteSelected();
    }, LV_EVENT_CLICKED, this);
}

// ─────────────────────────────────────────────────────────────────────────────
//  View switching
// ─────────────────────────────────────────────────────────────────────────────

void ScreenGallery::showGrid() {
    lv_obj_clear_flag(grid_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(detail_view, LV_OBJ_FLAG_HIDDEN);
    selected_idx = -1;
}

void ScreenGallery::showDetail(int idx) {
    if (idx < 0 || idx >= (int)entries.size()) return;
    selected_idx = idx;

    const PhotoEntry& e = entries[idx];

    lv_label_set_text(detail_status, "Loading...");
    lv_obj_add_flag(grid_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(detail_view, LV_OBJ_FLAG_HIDDEN);

    // Load the full BMP into PSRAM
    int fw = 0, fh = 0;
    uint8_t* pxbuf = loadBmpFull(e.path.c_str(), &fw, &fh);

    if (!pxbuf) {
        lv_label_set_text(detail_status, "Load failed");
        return;
    }

    lv_label_set_text(detail_status, "");

    // Use the full screen as the canvas area.
    // The header bar (drawn on top via z-order) covers HEADER_H at the top;
    // the footer bar inside detail_view covers FOOTER_H at the bottom.
    // The image is positioned to fill the entire screen.
    int canvas_area_w = SCREEN_W;
    int canvas_area_h = SCREEN_H;

    // Scale to fit while preserving aspect ratio
    int disp_w = fw, disp_h = fh;
    if (disp_w > canvas_area_w) {
        disp_h = disp_h * canvas_area_w / disp_w;
        disp_w = canvas_area_w;
    }
    if (disp_h > canvas_area_h) {
        disp_w = disp_w * canvas_area_h / disp_h;
        disp_h = canvas_area_h;
    }

    static uint8_t* selected_buf = nullptr;
    if (selected_buf) { free(selected_buf); selected_buf = nullptr; }
    selected_buf = pxbuf;

    // Bind the full buffer (fw x fh) to the canvas, then size the widget to
    // disp_w x disp_h.  When disp == fw x fh (same-size image) LVGL shows the
    // whole image; if scaled down it will clip — acceptable for oversized files.
    lv_canvas_set_buffer(detail_canvas, selected_buf, fw, fh, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_size(detail_canvas, disp_w, disp_h);
    lv_obj_set_pos(detail_canvas,
                   (canvas_area_w - disp_w) / 2,
                   (canvas_area_h - disp_h) / 2);
    lv_obj_invalidate(detail_canvas);

    Serial.printf("[GALLERY] Detail showing: %s  %dx%d\n",
                  e.path.c_str(), fw, fh);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Delete selected photo
// ─────────────────────────────────────────────────────────────────────────────

void ScreenGallery::deleteSelected() {
    if (selected_idx < 0 || selected_idx >= (int)entries.size()) return;

    PhotoEntry& e = entries[selected_idx];
    lv_label_set_text(detail_status, "Deleting...");

    bool ok = storage_delete_file(e.path.c_str());
    if (!ok) {
        lv_label_set_text(detail_status, "Delete failed!");
        return;
    }

    // Remove from all_photos
    int global_idx = (current_page * PAGE_SIZE) + selected_idx;
    if (global_idx < (int)all_photos.size()) {
        all_photos.erase(all_photos.begin() + global_idx);
    }

    Serial.printf("[GALLERY] Deleted entry %d, %d remaining globally\n",
                  selected_idx, (int)all_photos.size());

    // If we just deleted the last photo on this page, and it's not the first page, go back a page.
    int total_pages = (all_photos.size() + PAGE_SIZE - 1) / PAGE_SIZE;
    if (current_page >= total_pages && current_page > 0) {
        current_page--;
    }

    // Return to grid and reload the current page to shift remaining items up
    showGrid();
    loadPage(current_page);
}

// ─────────────────────────────────────────────────────────────────────────────
//  SD scan + thumbnail grid population
// ─────────────────────────────────────────────────────────────────────────────

void ScreenGallery::loadFromSD() {
    Serial.println("[GALLERY] Scanning SD card...");

    all_photos.clear();

    File dir = SD_MMC.open("/");
    if (!dir || !dir.isDirectory()) {
        Serial.println("[GALLERY] Cannot open SD root /");
        return;
    }

    File f = dir.openNextFile();
    while (f) {
        if (!f.isDirectory()) {
            String name = String(f.name());
            String lower = name;
            lower.toLowerCase();
            if (lower.endsWith(".bmp") && lower.startsWith("photo_")) {
                PhotoFile pf;
                pf.path = name.startsWith("/") ? name.c_str() : ("/" + name).c_str();

                // Extract numeric ID from photo_1234.bmp
                int start = pf.path.find('_') + 1;
                int end = pf.path.find('.');
                if (start > 0 && end > start) {
                    std::string num_str = pf.path.substr(start, end - start);
                    pf.id = strtoul(num_str.c_str(), nullptr, 10);
                } else {
                    pf.id = 0;
                }
                all_photos.push_back(pf);
            }
        }
        f = dir.openNextFile();
    }

    Serial.printf("[GALLERY] Scan complete: %d photo(s) found.\n", (int)all_photos.size());

    // Sort descending by ID (newest first)
    std::sort(all_photos.begin(), all_photos.end(), [](const PhotoFile& a, const PhotoFile& b) {
        return a.id > b.id;
    });

    current_page = 0;
    loadPage(current_page);
}

void ScreenGallery::loadPage(int page) {
    Serial.printf("[GALLERY] Loading page %d...\n", page);

    // Clear old UI thumbnails
    lv_obj_clean(grid_view);

    // Free old PSRAM descriptors
    for (auto& e : entries) {
        if (e.thumb) { free(e.thumb); e.thumb = nullptr; }
    }
    entries.clear();

    int start_idx = page * PAGE_SIZE;
    int end_idx = std::min(start_idx + PAGE_SIZE, (int)all_photos.size());

    for (int i = start_idx; i < end_idx; i++) {
        const std::string& full = all_photos[i].path;
        int sw = 0, sh = 0;
        lv_img_dsc_t* dsc = loadBmpThumbnail(full.c_str(), &sw, &sh);
        if (dsc) {
            PhotoEntry entry;
            entry.path  = full;
            entry.thumb = dsc;
            entry.src_w = sw;
            entry.src_h = sh;

            int entry_idx = (int)entries.size();
            entries.push_back(entry);

            // ── Thumbnail card ───────────────────────────────────────
            int tw = sw / THUMB_SCALE;
            int th = sh / THUMB_SCALE;
            int card_w = tw + 8;
            int card_h = th + 8;

            lv_obj_t* card = lv_obj_create(grid_view);
            lv_obj_add_style(card, &Theme::card, 0);
            lv_obj_set_size(card, card_w, card_h);
            lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

            lv_obj_t* img = lv_img_create(card);
            lv_img_set_src(img, dsc);
            lv_obj_center(img);

            lv_obj_add_event_cb(card, [](lv_event_t* e) {
                ScreenGallery* self = static_cast<ScreenGallery*>(lv_event_get_user_data(e));
                lv_obj_t* card  = lv_event_get_target(e);
                lv_obj_t* grid  = lv_obj_get_parent(card);
                uint32_t  cnt   = lv_obj_get_child_cnt(grid);
                for (uint32_t i = 0; i < cnt; i++) {
                    if (lv_obj_get_child(grid, i) == card) {
                        self->showDetail((int)i);
                        return;
                    }
                }
            }, LV_EVENT_CLICKED, this);
        }
    }

    if (entries.empty()) {
        lv_obj_t* lbl = lv_label_create(grid_view);
        lv_obj_add_style(lbl, &Theme::text_body, 0);
        lv_label_set_text(lbl, "No photos on SD card");
        lv_obj_center(lbl);
    }

    updatePaginationUI();
}

void ScreenGallery::updatePaginationUI() {
    if (!grid_footer) return;

    int total_pages = (all_photos.size() + PAGE_SIZE - 1) / PAGE_SIZE;
    if (total_pages == 0) total_pages = 1;

    char buf[32];
    snprintf(buf, sizeof(buf), "%d / %d", current_page + 1, total_pages);
    if (page_label) {
        lv_label_set_text(page_label, buf);
    }

    if (btn_prev) {
        if (current_page == 0) lv_obj_add_state(btn_prev, LV_STATE_DISABLED);
        else lv_obj_clear_state(btn_prev, LV_STATE_DISABLED);
    }
    if (btn_next) {
        if (current_page >= total_pages - 1) lv_obj_add_state(btn_next, LV_STATE_DISABLED);
        else lv_obj_clear_state(btn_next, LV_STATE_DISABLED);
    }
}

} // namespace ui
