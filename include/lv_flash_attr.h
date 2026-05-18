/**
 * lv_flash_attr.h
 *
 * Forces all LVGL-generated large image/font pixel arrays into flash (rodata)
 * instead of RAM. Without this, arrays marked LV_ATTRIBUTE_LARGE_CONST end up
 * in .data (copied to RAM at boot), consuming ~150KB of precious SRAM.
 *
 * Include via: build_flags = -include include/lv_flash_attr.h
 * Must be included BEFORE any LVGL headers.
 */
#pragma once

#ifndef LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_CONST  __attribute__((section(".rodata")))
#endif

#ifndef LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY  /* empty: let linker decide */
#endif
