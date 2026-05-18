#pragma once
// =============================================================================
// core/app.h — App-level state machine
// =============================================================================

enum AppState {
    STATE_VIEWFINDER,
    STATE_FILTERS,
    STATE_COUNTDOWN,
    STATE_FEEDBACK,
    STATE_PREVIEW,
    STATE_SETTINGS,
    STATE_GALLERY,
    STATE_SHARE,        // WiFi AP gallery sharing
    STATE_ABOUT,
};


extern AppState app_state;
extern int      app_timer_duration;           // 0 = Off, or 3, 5, 10 seconds
extern bool     app_photobooth_style_capture; // true = capture whole screen

void app_init();
void app_set_state(AppState next);
AppState app_get_state();
