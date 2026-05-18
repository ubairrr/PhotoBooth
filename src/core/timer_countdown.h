#pragma once
// =============================================================================
// core/timer_countdown.h — Countdown timer logic (Phase 4)
// =============================================================================
#include <stdint.h>

void countdown_start(uint8_t seconds);
void countdown_stop();
bool countdown_is_running();
uint8_t countdown_value();          // current seconds remaining
