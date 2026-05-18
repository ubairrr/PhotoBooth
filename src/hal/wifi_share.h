#pragma once
// =============================================================================
// hal/wifi_share.h  —  WiFi Access-Point + async HTTP photo gallery server
// =============================================================================
#include <Arduino.h>

// ── AP credentials (hardcoded — no keyboard on device) ──────────────────────
static constexpr char SHARE_SSID[]     = "Maryam's Gallery";
static constexpr char SHARE_PASSWORD[] = "photobooth";
static constexpr char SHARE_URL[]      = "http://192.168.4.1";
static constexpr uint16_t SHARE_PORT   = 80;

/**
 * Start the WiFi soft-AP and HTTP server.
 * Scans the SD card root for all *.bmp files and builds an in-memory index.
 * Call once when entering STATE_SHARE.
 */
void wifi_share_start();

/**
 * Stop the HTTP server and bring down the WiFi AP.
 * Frees all allocated resources.
 * Call when leaving STATE_SHARE.
 */
void wifi_share_stop();
