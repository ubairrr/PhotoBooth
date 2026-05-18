// =============================================================================
// hal/boot_anim.cpp — MJPEG boot animation via TFT_eSPI + JPEGDEC
// =============================================================================
//
// HOW IT WORKS
// ─────────────
// The MJPEG video is embedded as a PROGMEM C-array (boot_anim_data.h).
// We scan the array for 0xFF 0xD8 (SOI) and 0xFF 0xD9 (EOI) markers to 
// extract each standalone JPEG frame. 
// We pass the pointer and size directly to JPEGDEC (openFLASH), which 
// decodes and blits the RGB565 pixels directly to the ILI9341 via TFT_eSPI 
// — no SD card, no LVGL, and no PSRAM buffers needed.
// =============================================================================

#include "boot_anim.h"
#include "display.h"       // extern TFT_eSPI tft
#include <Arduino.h>
#include <JPEGDEC.h>
#include <new>
#include "../assets/boot_anim_data.h"

// ---------------------------------------------------------------------------
// JPEGDEC draw callback
// Called for every decoded 16×16 MCU block; pushes it straight to the display.
// ---------------------------------------------------------------------------
static int jpeg_draw_cb(JPEGDRAW* pDraw) {
    // pDraw->pPixels is RGB565, big-endian — matches ILI9341 natively
    tft.pushImage(pDraw->x, pDraw->y,
                  pDraw->iWidth, pDraw->iHeight,
                  pDraw->pPixels);
    return 1;  // non-zero = continue
}

// ---------------------------------------------------------------------------
// boot_anim_play
// ---------------------------------------------------------------------------
void boot_anim_play(uint8_t fps) {
    if (fps == 0 || boot_anim_len == 0) return;

    Serial.printf("[BOOT_ANIM] Playing from Flash (%u bytes, %u fps)\n",
                  (unsigned)boot_anim_len, fps);

    // ── Allocate JPEGDEC state in PSRAM ──────────────────────────────────────
    void* jpeg_mem = ps_malloc(sizeof(JPEGDEC));
    if (!jpeg_mem) {
        Serial.println("[BOOT_ANIM] FATAL: failed to allocate JPEGDEC in PSRAM");
        return;
    }
    JPEGDEC* jpeg = new (jpeg_mem) JPEGDEC();

    // ── Timing ───────────────────────────────────────────────────────────────
    const uint32_t frame_ms = 1000u / fps;

    // ── Prepare display for direct pixel writes ───────────────────────────────
    tft.startWrite();

    // ── Frame extraction and playback ────────────────────────────────────────
    uint32_t frameCount = 0;
    uint32_t t0 = millis();

    size_t i = 0;
    while (i < boot_anim_len - 1) {
        // Find Start of Image (SOI)
        if (boot_anim_data[i] == 0xFF && boot_anim_data[i+1] == 0xD8) {
            size_t frame_start = i;
            i += 2;
            
            // Find End of Image (EOI)
            while (i < boot_anim_len - 1) {
                if (boot_anim_data[i] == 0xFF && boot_anim_data[i+1] == 0xD9) {
                    size_t frame_end = i + 1;
                    size_t frame_size = (frame_end - frame_start) + 1;
                    
                    uint32_t frameStartMs = millis();
                    
                    // Decode & blit directly from Flash memory
                    if (jpeg->openFLASH((uint8_t*)&boot_anim_data[frame_start], frame_size, jpeg_draw_cb)) {
                        jpeg->setPixelType(RGB565_BIG_ENDIAN);
                        jpeg->decode(0, 0, 0);
                        jpeg->close();
                    }
                    
                    // Frame-rate pacing
                    uint32_t elapsed = millis() - frameStartMs;
                    if (elapsed < frame_ms) {
                        delay(frame_ms - elapsed);
                    }
                    
                    frameCount++;
                    i += 2; // Move past EOI
                    break;
                }
                i++;
            }
        } else {
            i++;
        }
    }

    // ── Done ─────────────────────────────────────────────────────────────────
    tft.endWrite();
    jpeg->~JPEGDEC();
    free(jpeg_mem);

    uint32_t totalMs = millis() - t0;
    Serial.printf("[BOOT_ANIM] Done — %u frames in %u ms (avg %.1f fps)\n",
                  frameCount, totalMs,
                  frameCount > 0 ? frameCount * 1000.0f / totalMs : 0.0f);
}
