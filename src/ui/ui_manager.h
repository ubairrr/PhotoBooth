#pragma once
#include "lvgl.h"
#include "screen_main.h"
#include "screen_filters.h"
#include "screen_preview.h"
#include "screen_gallery.h"
#include "screen_settings.h"
#include "screen_share.h"
#include "screen_about.h"

namespace ui {

enum class ScreenID {
    MAIN,
    FILTERS,
    PREVIEW,
    GALLERY,
    SETTINGS,
    SHARE,
    ABOUT,
};


class UIManager {
public:
    static UIManager& get();

    void init();
    void navigate(ScreenID target);

    ScreenMain mainScreen;
    ScreenFilters filtersScreen;
    ScreenPreview previewScreen;
    ScreenGallery galleryScreen;
    ScreenSettings settingsScreen;
    ScreenShare shareScreen;
    ScreenAbout aboutScreen;


private:
    UIManager() = default;

    lv_obj_t* current = nullptr;
    ScreenID currentID = ScreenID::MAIN;
};

}
