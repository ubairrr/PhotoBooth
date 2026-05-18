#include "screen_filters.h"
#include <Arduino.h>
#include "theme.h"
#include "ui_helpers.h"
#include "ui_manager.h"
#include "../../core/app.h"

LV_IMG_DECLARE(cat_idle);

namespace ui {

static constexpr int SCREEN_W = 240;
static constexpr int SCREEN_H = 320;
static constexpr int HEADER_H = 42;

static void on_close_click(lv_event_t* e) {
    (void)e;
    app_set_state(STATE_VIEWFINDER);
}

static void on_filter_item_click(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    FilterType type = (FilterType)(uintptr_t)lv_obj_get_user_data(btn);
    
    current_filter = type;
    app_set_state(STATE_VIEWFINDER); // Return to viewfinder immediately
}

lv_obj_t* ScreenFilters::create() {
    root = lv_obj_create(NULL);
    lv_obj_add_style(root, &Theme::screen_bg, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    buildHeader();
    buildGrid();

    return root;
}

void ScreenFilters::destroy() {
    if (root) {
        lv_obj_del(root);
        root = nullptr;
        grid = nullptr;
    }
    
    for (auto dsc : filtered_images) {
        if (dsc->data) free((void*)dsc->data);
        free(dsc);
    }
    filtered_images.clear();
}

//////////////////////////////////////////////////////////////
// HEADER
//////////////////////////////////////////////////////////////
void ScreenFilters::buildHeader() {
    lv_obj_t* header = lv_obj_create(root);
    lv_obj_remove_style_all(header);
    lv_obj_add_style(header, &Theme::top_bar, 0);

    lv_obj_set_size(header, SCREEN_W, HEADER_H);

    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    create_label(header, &Theme::text_cursive,
                 "Filters", LV_ALIGN_LEFT_MID);

    lv_obj_t* btn_close = create_button(header,
                                        &Theme::btn_secondary,
                                        34, 34,
                                        LV_ALIGN_RIGHT_MID);

    create_label(btn_close, &Theme::text_body,
                 "X", LV_ALIGN_CENTER);

    lv_obj_add_event_cb(btn_close, on_close_click, LV_EVENT_CLICKED, NULL);
}

//////////////////////////////////////////////////////////////
// GRID
//////////////////////////////////////////////////////////////
void ScreenFilters::buildGrid() {
    grid = lv_obj_create(root);
    lv_obj_remove_style_all(grid);

    lv_obj_set_size(grid, SCREEN_W, SCREEN_H - HEADER_H);

    lv_obj_set_scroll_dir(grid, LV_DIR_VER);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);

    lv_obj_set_style_pad_row(grid, 8, 0);
    lv_obj_set_style_pad_column(grid, 8, 0);
    lv_obj_set_style_pad_all(grid, 8, 0);

    createFilterItem(grid, "Normal", FilterType::NONE);
    createFilterItem(grid, "Vivid", FilterType::VIVID);
    createFilterItem(grid, "Blush", FilterType::BLUSH);
    createFilterItem(grid, "B & W", FilterType::GRAYSCALE);
    createFilterItem(grid, "Sepia", FilterType::SEPIA);
    createFilterItem(grid, "Invert", FilterType::INVERT);
    createFilterItem(grid, "Warm", FilterType::WARM);
    createFilterItem(grid, "Cool", FilterType::COOL);
}

//////////////////////////////////////////////////////////////
// FILTER ITEM
//////////////////////////////////////////////////////////////
lv_obj_t* ScreenFilters::createFilterItem(lv_obj_t* parent,
                                          const char* name, FilterType type) {

    lv_obj_t* item = lv_btn_create(parent);
    lv_obj_remove_style_all(item); // Remove default button styles
    lv_obj_add_style(item, &Theme::card, 0);
    lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE); // Make it functional!
    lv_obj_set_user_data(item, (void*)(uintptr_t)type);
    lv_obj_add_event_cb(item, on_filter_item_click, LV_EVENT_CLICKED, NULL);

    lv_obj_set_size(item, 104, 86);

    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(item,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    // Thumbnail
    lv_obj_t* thumb = lv_img_create(item);
    
    lv_img_dsc_t* dsc = (lv_img_dsc_t*)ps_malloc(sizeof(lv_img_dsc_t));
    if (!dsc) dsc = (lv_img_dsc_t*)malloc(sizeof(lv_img_dsc_t));
    
    if (dsc) {
        memcpy(dsc, &cat_idle, sizeof(lv_img_dsc_t));
        uint8_t* dst_data = (uint8_t*)ps_malloc(cat_idle.data_size);
        if (!dst_data) dst_data = (uint8_t*)malloc(cat_idle.data_size);
        
        if (dst_data) {
            const uint8_t* src_data = cat_idle.data;
            for (uint32_t i = 0; i < cat_idle.data_size; i += 3) {
                lv_color_t src_color;
                src_color.full = src_data[i] | (src_data[i+1] << 8);
                uint8_t alpha = src_data[i+2];

                lv_color_t dst_color;
                apply_filter(&dst_color, &src_color, 1, type);

                dst_data[i] = dst_color.full & 0xFF;
                dst_data[i+1] = (dst_color.full >> 8) & 0xFF;
                dst_data[i+2] = alpha;
            }
            dsc->data = dst_data;
            lv_img_set_src(thumb, dsc);
            filtered_images.push_back(dsc);
        } else {
            free(dsc);
            lv_img_set_src(thumb, &cat_idle);
        }
    } else {
        lv_img_set_src(thumb, &cat_idle);
    }

    lv_obj_clear_flag(thumb, LV_OBJ_FLAG_CLICKABLE); // Ensure clicks pass through to parent button

    // Label
    create_label(item, &Theme::text_body,
                 name, LV_ALIGN_CENTER);

    return item;
}

} // namespace ui
