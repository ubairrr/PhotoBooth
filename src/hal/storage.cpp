// =============================================================================
// hal/storage.cpp — MicroSD card read/write via SDMMC
// =============================================================================
#include "storage.h"
#include "SD_MMC.h"

// SDMMC pins (1-bit mode)
#define SDMMC_CMD 38
#define SDMMC_CLK 39
#define SDMMC_D0  40

static bool sd_ready = false;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void write_u16_le(File& file, uint16_t v) {
    uint8_t b[2] = { uint8_t(v), uint8_t(v >> 8) };
    file.write(b, 2);
}

static void write_u32_le(File& file, uint32_t v) {
    uint8_t b[4] = { uint8_t(v), uint8_t(v >> 8), uint8_t(v >> 16), uint8_t(v >> 24) };
    file.write(b, 4);
}

static unsigned long current_max_photo_id = 0;

// ---------------------------------------------------------------------------
// storage_init
// ---------------------------------------------------------------------------
bool storage_init() {
    // Give the SD card a moment after power-on
    delay(100);

    SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0);
    // begin(mountpoint, mode1bit, format_if_empty, freq_khz, max_files)
    sd_ready = SD_MMC.begin("/sdcard", true);
    
    if (!sd_ready) {
        Serial.println("[SD] Init FAILED — no card or wiring error");
        return false;
    }

    Serial.printf("[SD] Ready — card type: %d\n", (int)SD_MMC.cardType());

    // Scan for the highest photo ID so we can continue sequentially
    File dir = SD_MMC.open("/");
    if (dir && dir.isDirectory()) {
        File f = dir.openNextFile();
        while (f) {
            if (!f.isDirectory()) {
                String name = String(f.name());
                String lower = name;
                lower.toLowerCase();
                if (lower.endsWith(".bmp") && lower.startsWith("photo_")) {
                    int start = name.indexOf('_') + 1;
                    int end = name.indexOf('.');
                    if (start > 0 && end > start) {
                        unsigned long id = strtoul(name.substring(start, end).c_str(), nullptr, 10);
                        if (id > current_max_photo_id) {
                            current_max_photo_id = id;
                        }
                    }
                }
            }
            f = dir.openNextFile();
        }
    }
    Serial.printf("[SD] Max photo ID found: %lu\n", current_max_photo_id);

    return true;
}

unsigned long storage_get_next_photo_id() {
    return ++current_max_photo_id;
}

// ---------------------------------------------------------------------------
// storage_save_jpeg
// ---------------------------------------------------------------------------
bool storage_save_jpeg(const uint8_t* data, size_t len, const char* filename) {
    if (!sd_ready || !data || !filename || len == 0) {
        Serial.println("[SD] save_jpeg: invalid args or SD not ready");
        return false;
    }

    File file = SD_MMC.open(filename, FILE_WRITE);
    if (!file) {
        Serial.printf("[SD] Open failed: %s\n", filename);
        return false;
    }

    size_t written = file.write(data, len);
    file.close();
    Serial.printf("[SD] Saved %s (%u/%u bytes)\n", filename, written, len);
    return written == len;
}

// ---------------------------------------------------------------------------
// storage_save_rgb565_bmp
// Converts RGB565 → 24-bit BGR BMP (standard Windows BMP, bottom-up rows).
// ---------------------------------------------------------------------------
bool storage_save_rgb565_bmp(const uint8_t* data, int width, int height,
                              const char* filename) {
    if (!sd_ready || !data || !filename || width <= 0 || height <= 0) {
        Serial.println("[SD] save_bmp: invalid args or SD not ready");
        return false;
    }

    // BMP rows must be padded to a multiple of 4 bytes
    const uint32_t row_size   = static_cast<uint32_t>((width * 3 + 3) & ~3);
    const uint32_t image_size = row_size * static_cast<uint32_t>(height);
    const uint32_t file_size  = 54 + image_size;

    File file = SD_MMC.open(filename, FILE_WRITE);
    if (!file) {
        Serial.printf("[SD] Open failed: %s\n", filename);
        return false;
    }

    // ── BMP file header (14 bytes) ────────────────────────────────────────
    file.write(uint8_t('B'));
    file.write(uint8_t('M'));
    write_u32_le(file, file_size);
    write_u16_le(file, 0);          // reserved
    write_u16_le(file, 0);          // reserved
    write_u32_le(file, 54);         // pixel data offset

    // ── DIB / BITMAPINFOHEADER (40 bytes) ────────────────────────────────
    write_u32_le(file, 40);
    write_u32_le(file, static_cast<uint32_t>(width));
    write_u32_le(file, static_cast<uint32_t>(height));
    write_u16_le(file, 1);          // color planes
    write_u16_le(file, 24);         // bits per pixel
    write_u32_le(file, 0);          // compression: BI_RGB
    write_u32_le(file, image_size);
    write_u32_le(file, 2835);       // X pix/metre (~72 DPI)
    write_u32_le(file, 2835);       // Y pix/metre
    write_u32_le(file, 0);          // colors in table
    write_u32_le(file, 0);          // important colors

    // ── Pixel rows (bottom-up, BGR order) ────────────────────────────────
    uint8_t* row = static_cast<uint8_t*>(ps_malloc(row_size));
    if (!row) {
        file.close();
        Serial.println("[SD] Row malloc failed");
        return false;
    }

    const uint16_t* pixels = reinterpret_cast<const uint16_t*>(data);
    for (int y = height - 1; y >= 0; y--) {
        memset(row, 0, row_size);
        for (int x = 0; x < width; x++) {
            uint16_t p     = pixels[y * width + x];
            // RGB565 → BGR888
            row[x * 3 + 0] = (p & 0x1F)         * 255 / 31;   // B
            row[x * 3 + 1] = ((p >> 5) & 0x3F)  * 255 / 63;   // G
            row[x * 3 + 2] = ((p >> 11) & 0x1F) * 255 / 31;   // R
        }
        if (file.write(row, row_size) != row_size) {
            free(row);
            file.close();
            Serial.printf("[SD] Write error at row %d: %s\n", y, filename);
            return false;
        }
        // Yield every 16 rows to keep the watchdog timer happy
        if ((y & 0xF) == 0) {
            yield();
        }
    }

    free(row);
    file.close();
    Serial.printf("[SD] Saved %s (%dx%d BMP, %u bytes)\n",
                  filename, width, height, file_size);
    return true;
}

// ---------------------------------------------------------------------------
// storage_delete_file
// ---------------------------------------------------------------------------
bool storage_delete_file(const char* filename) {
    if (!sd_ready || !filename) {
        Serial.println("[SD] delete: invalid args or SD not ready");
        return false;
    }
    if (!SD_MMC.exists(filename)) {
        Serial.printf("[SD] delete: file not found: %s\n", filename);
        return false;
    }
    bool ok = SD_MMC.remove(filename);
    if (ok) {
        Serial.printf("[SD] Deleted: %s\n", filename);
    } else {
        Serial.printf("[SD] Delete FAILED: %s\n", filename);
    }
    return ok;
}
