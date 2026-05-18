#pragma once
// =============================================================================
// hal/storage.h — MicroSD card read/write via SPI
// =============================================================================
#include <Arduino.h>

/**
 * Initialise the SD card over SPI.
 * Returns true if the card was found and mounted successfully.
 */
bool storage_init();

/**
 * Returns a monotonic sequential ID for the next photo.
 */
unsigned long storage_get_next_photo_id();

/**
 * Save a raw JPEG byte-stream to the SD card.
 */
bool storage_save_jpeg(const uint8_t* data, size_t len, const char* filename);

/**
 * Save an RGB565 framebuffer as a 24-bit BMP to the SD card.
 * 'data' must point to width*height*2 bytes of packed RGB565 pixels.
 */
bool storage_save_rgb565_bmp(const uint8_t* data, int width, int height,
                             const char* filename);

/**
 * Delete a file from the SD card.
 * Returns true if the file was deleted, false on error.
 */
bool storage_delete_file(const char* filename);
