// =============================================================================
// core/timer_countdown.cpp — Countdown timer logic (Phase 4 stub)
// =============================================================================
#include "timer_countdown.h"
#include <Arduino.h>

static uint8_t  _seconds  = 0;
static bool     _running  = false;
static uint32_t _tick_ms  = 0;

void countdown_start(uint8_t seconds) {
    _seconds = seconds;
    _running = true;
    _tick_ms = millis();
    Serial.printf("[COUNTDOWN] Started: %ds\n", seconds);
}

void countdown_stop() {
    _running = false;
    _seconds = 0;
}

bool countdown_is_running() { return _running; }

uint8_t countdown_value() {
    if (!_running) return 0;
    uint32_t elapsed = (millis() - _tick_ms) / 1000;
    if (elapsed >= _seconds) { _running = false; return 0; }
    return _seconds - (uint8_t)elapsed;
}
