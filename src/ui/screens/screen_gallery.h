// =============================================================================
// screen_gallery.h  —  SD-card photo browser
//
// Two-view design:
//   VIEW_GRID    : scrollable thumbnail grid (all BMP files on SD root)
//   VIEW_DETAIL  : fullscreen viewer for a selected photo + Delete button
// =============================================================================
#pragma once
#include "lvgl.h"
#include <vector>
#include <string>

namespace ui {

class ScreenGallery {
public:
    lv_obj_t* create();
    void       destroy();

private:
    // ── Per-photo entry ──────────────────────────────────────────────────────
    struct PhotoEntry {
        std::string    path;        // "/photo_12345.bmp" (absolute SD path)
        lv_img_dsc_t*  thumb;       // PSRAM thumbnail (heap-alloc'd, owned here)
        int            src_w;       // original BMP width  (read from header)
        int            src_h;       // original BMP height
    };

    // ── State ────────────────────────────────────────────────────────────────
    lv_obj_t*               root        = nullptr;

    // Grid view
    lv_obj_t*               grid_view   = nullptr;   // scrollable row-wrap flex

    // Detail view
    lv_obj_t*               detail_view = nullptr;   // hidden until a thumb is tapped
    lv_obj_t*               detail_canvas = nullptr;
    lv_obj_t*               detail_status = nullptr;

    // ── Pagination State ─────────────────────────────────────────────────────
    struct PhotoFile {
        std::string path;
        unsigned long id;
    };
    std::vector<PhotoFile>  all_photos;
    int                     current_page = 0;
    static constexpr int    PAGE_SIZE = 12;

    std::vector<PhotoEntry> entries;
    int                     selected_idx = -1;

    // Pagination UI
    lv_obj_t*               grid_footer  = nullptr;
    lv_obj_t*               page_label   = nullptr;
    lv_obj_t*               btn_prev     = nullptr;
    lv_obj_t*               btn_next     = nullptr;

    // ── Helpers ──────────────────────────────────────────────────────────────
    void buildHeader();
    void buildGridView();
    void buildDetailView();

    void loadFromSD();
    void loadPage(int page);
    void updatePaginationUI();
    void showGrid();
    void showDetail(int idx);
    void deleteSelected();

    // BMP thumbnail loader: handles any BMP the photobooth may have written.
    // Returns heap-alloc'd lv_img_dsc_t* on success, nullptr on failure.
    static lv_img_dsc_t* loadBmpThumbnail(const char* path, int* out_w, int* out_h);

    // Load a BMP fully into a PSRAM buffer for the detail canvas.
    // Returns ps_malloc'd pixel buffer (RGB565 LE) on success.
    static uint8_t* loadBmpFull(const char* path, int* out_w, int* out_h);
};

} // namespace ui