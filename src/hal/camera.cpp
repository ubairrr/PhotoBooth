// =============================================================================
// hal/camera.cpp — OV3660 live viewfinder + PSRAM capture cache
// =============================================================================
#include "camera.h"

// Camera pin definitions (OV3660 on custom board)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM   4
#define SIOC_GPIO_NUM   5
#define Y9_GPIO_NUM    16
#define Y8_GPIO_NUM    17
#define Y7_GPIO_NUM    18
#define Y6_GPIO_NUM    12
#define Y5_GPIO_NUM    10
#define Y4_GPIO_NUM     8
#define Y3_GPIO_NUM     9
#define Y2_GPIO_NUM    11
#define VSYNC_GPIO_NUM  6
#define HREF_GPIO_NUM   7
#define PCLK_GPIO_NUM  13

// ---------------------------------------------------------------------------
// Shared state definitions
// ---------------------------------------------------------------------------
lv_color_t* preview_buf   = nullptr;
lv_color_t* canvas_buf    = nullptr;
lv_color_t* captured_buf  = nullptr;
int         captured_w    = 0;
int         captured_h    = 0;

volatile bool frame_ready       = false;
volatile bool capture_requested = false;
volatile bool capture_filtered  = false;

// Internal flag: set by camera_task once it has filled captured_buf
static volatile bool capture_done = false;

// ---------------------------------------------------------------------------
// camera_init
// ---------------------------------------------------------------------------
void camera_init() {
    camera_config_t config;
    config.ledc_channel  = LEDC_CHANNEL_0;
    config.ledc_timer    = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size   = FRAMESIZE_QVGA;    // 320x240 (Raw Sensor)
    config.grab_mode    = CAMERA_GRAB_LATEST;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.fb_count     = 2;

    if (esp_camera_init(&config) != ESP_OK) {
        Serial.println("[CAM] FATAL: init failed");
        return;
    }

    sensor_t * s = esp_camera_sensor_get();
    if (s) {
        // Force the sensor into a native 240x320 Portrait window.
        // We take a portrait crop from the high-res sensor array to avoid squeezing.
        // Parameters: startX, startY, endX, endY, offsetX, offsetY, totalX, totalY, outputX, outputY, scale, binning
        s->set_res_raw(s, 448, 0, 1599, 1535, 0, 0, 1152, 1536, 240, 320, true, true);
        
        s->set_vflip(s, 1);
        s->set_hmirror(s, 1);
    }

    // Allocate double PSRAM buffers for the 240x320 Portrait Pipeline
    canvas_buf  = (lv_color_t*)ps_malloc(DISP_W * DISP_H * sizeof(lv_color_t));
    preview_buf = (lv_color_t*)ps_malloc(DISP_W * DISP_H * sizeof(lv_color_t));
    
    if (!canvas_buf || !preview_buf) {
        Serial.println("[CAM] FATAL: PSRAM alloc failed for portrait buffers");
        return;
    }
    memset(canvas_buf,  0, DISP_W * DISP_H * sizeof(lv_color_t));
    memset(preview_buf, 0, DISP_W * DISP_H * sizeof(lv_color_t));

    Serial.println("[CAM] Portrait Pipeline Initialized (240x320)");
}

// ---------------------------------------------------------------------------
// Public capture API (called from Core 1 / LVGL loop)
// ---------------------------------------------------------------------------
void camera_capture() {
    capture_done      = false;
    capture_filtered  = false;
    capture_requested = true;   // signal camera_task to freeze next frame
    Serial.println("[CAM] Capture requested");
}

bool camera_has_capture() {
    return capture_done;
}

void camera_clear_cache() {
    if (captured_buf) {
        free(captured_buf);
        captured_buf = nullptr;
    }
    captured_w    = 0;
    captured_h    = 0;
    capture_done  = false;
    Serial.println("[CAM] Capture cache cleared");
}

// ---------------------------------------------------------------------------
// Native Portrait Copy (Fast)
// Since the sensor now outputs 240x320 directly, we just need to copy
// and perform a byte-swap to match the display's RGB565 endianness.
// ---------------------------------------------------------------------------
void copy_portrait_frame(camera_fb_t* fb, lv_color_t* dst_buf) {
    if (!fb || !dst_buf) return;

    uint16_t* src = (uint16_t*)fb->buf;
    uint16_t* dst = (uint16_t*)dst_buf;
    size_t pixel_count = DISP_W * DISP_H;

    // Fast byte-swap loop (Big Endian -> Little Endian)
    for (size_t i = 0; i < pixel_count; i++) {
        uint16_t p = src[i];
        dst[i] = (p >> 8) | (p << 8);
    }
}

// ---------------------------------------------------------------------------
// camera_task  (Core 0)
// ---------------------------------------------------------------------------
void camera_task(void* pvParameters) {
    uint32_t frame_count    = 0;
    uint32_t last_fps_time  = millis();
    uint32_t last_frame_ms  = 0;

    while (1) {
        // ── Capture request: freeze one frame into PSRAM cache ──────────────
        if (capture_requested) {
            camera_fb_t* snap = esp_camera_fb_get();
            if (snap) {
                size_t buf_size = DISP_W * DISP_H * sizeof(lv_color_t);

                // Allocate (or reuse) capture cache in PSRAM
                if (!captured_buf) {
                    captured_buf = (lv_color_t*)ps_malloc(buf_size);
                }

                if (captured_buf) {
                    copy_portrait_frame(snap, captured_buf);
                    captured_w   = 240;
                    captured_h   = 320;
                    capture_done = true;   // signal Core 1 that the frame is ready
                    Serial.printf("[CAM] Snapshot cached — %dx%d in PSRAM\n",
                                  captured_w, captured_h);
                } else {
                    Serial.println("[CAM] ERROR: ps_malloc failed for capture cache");
                }
                esp_camera_fb_return(snap);
            } else {
                Serial.println("[CAM] ERROR: failed to get camera frame for capture");
            }

            capture_requested = false;  // clear flag regardless
            continue;
        }

        // ── Live viewfinder: fill preview_buf at 25 FPS ────────────────────
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb) {
            if (preview_buf) {
                copy_portrait_frame(fb, preview_buf);
                uint32_t now = millis();
                if (now - last_frame_ms >= 40) {   // cap at ~25 FPS
                    frame_ready   = true;
                    last_frame_ms = now;
                }
            }
            esp_camera_fb_return(fb);

            // FPS counter
            frame_count++;
            uint32_t now = millis();
            if (now - last_fps_time >= 1000) {
                Serial.printf("[CAM] FPS: %u\n", frame_count);
                frame_count   = 0;
                last_fps_time = now;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// ---------------------------------------------------------------------------
void camera_start_task() {
    xTaskCreatePinnedToCore(camera_task, "cam_task", 8192, NULL, 1, NULL, 0);
    Serial.println("[CAM] Task started on Core 0");
}
