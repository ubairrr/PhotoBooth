#include "ui_manager.h"

namespace ui {

UIManager& UIManager::get() {
    static UIManager instance;
    return instance;
}

void UIManager::init() {
    current = mainScreen.create();
    currentID = ScreenID::MAIN;

    lv_scr_load(current);
}

void UIManager::navigate(ScreenID target) {

    if (target == currentID) return;

    ScreenID previousID = currentID;

    // Create new screen
    switch (target) {
        case ScreenID::MAIN:
            current = mainScreen.create();
            break;

        case ScreenID::FILTERS:
            current = filtersScreen.create();
            break;

        case ScreenID::PREVIEW:
            current = previewScreen.create();
            // Bind captured_buf to the canvas immediately (may be async if
            // camera_task hasn't finished copying the snapshot yet, but the
            // canvas will show the frame as soon as it is ready).
            previewScreen.showCapturedFrame();
            break;

        case ScreenID::GALLERY:
            current = galleryScreen.create();
            break;

        case ScreenID::SETTINGS:
            current = settingsScreen.create();
            break;

        case ScreenID::SHARE:
            current = shareScreen.create();
            break;
            
        case ScreenID::ABOUT:
            current = aboutScreen.create();
            break;
    }

    currentID = target;
    lv_scr_load(current);

    // Destroy previous screen after the new one is active.
    switch (previousID) {
        case ScreenID::MAIN:
            mainScreen.destroy();
            break;
        case ScreenID::FILTERS:
            filtersScreen.destroy();
            break;
        case ScreenID::PREVIEW:
            previewScreen.destroy();
            break;
        case ScreenID::GALLERY:
            galleryScreen.destroy();
            break;
        case ScreenID::SETTINGS:
            settingsScreen.destroy();
            break;
        case ScreenID::SHARE:
            shareScreen.destroy();
            break;
        case ScreenID::ABOUT:
            aboutScreen.destroy();
            break;
    }
}

}
