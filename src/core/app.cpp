// =============================================================================
// core/app.cpp — App state machine — wires screen transitions
// =============================================================================
#include "app.h"
#include <Arduino.h>
#include "../ui/ui_manager.h"

AppState app_state = STATE_VIEWFINDER;
int      app_timer_duration = 0; // Default to Off
bool     app_photobooth_style_capture = false;

static const char *state_name(AppState s) {
    switch (s) {
        case STATE_VIEWFINDER: return "VIEWFINDER";
        case STATE_FILTERS:    return "FILTERS";
        case STATE_COUNTDOWN:  return "COUNTDOWN";
        case STATE_FEEDBACK:   return "FEEDBACK";
        case STATE_PREVIEW:    return "PREVIEW";
        case STATE_SETTINGS:   return "SETTINGS";
        case STATE_GALLERY:    return "GALLERY";
        case STATE_SHARE:      return "SHARE";
        case STATE_ABOUT:      return "ABOUT";
        default:               return "UNKNOWN";
    }
}

void app_init() {
    app_state = STATE_VIEWFINDER;
    Serial.println("[APP] State machine initialised → VIEWFINDER");
}

void app_set_state(AppState next) {
    Serial.printf("[APP] %s → %s\n", state_name(app_state), state_name(next));
    app_state = next;

    switch (next) {
        case STATE_VIEWFINDER:
            ui::UIManager::get().navigate(ui::ScreenID::MAIN);
            break;
        case STATE_FILTERS:
            ui::UIManager::get().navigate(ui::ScreenID::FILTERS);
            break;
        case STATE_PREVIEW:
        case STATE_FEEDBACK:
            ui::UIManager::get().navigate(ui::ScreenID::PREVIEW);
            break;
        case STATE_GALLERY:
            ui::UIManager::get().navigate(ui::ScreenID::GALLERY);
            break;
        case STATE_SETTINGS:
            ui::UIManager::get().navigate(ui::ScreenID::SETTINGS);
            break;
        case STATE_SHARE:
            ui::UIManager::get().navigate(ui::ScreenID::SHARE);
            break;
        case STATE_ABOUT:
            ui::UIManager::get().navigate(ui::ScreenID::ABOUT);
            break;
        default:
            break;
    }
}

AppState app_get_state() { return app_state; }
