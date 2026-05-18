#if 1  /*Set it to "1" to enable content*/

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* Color depth: 16 = RGB565, matches ILI9341 natively */
#define LV_COLOR_DEPTH          16
#define LV_COLOR_16_SWAP         0   /* TFT_eSPI handles the swap via pushColors(..., true) */

/* Memory — 64KB heap is ample for a 240x320 widget tree; the rest of the
   draw budget (framebuffers etc.) lives in PSRAM, not here.              */
#define LV_MEM_CUSTOM            1
#define LV_MEM_CUSTOM_INCLUDE    <Arduino.h>   /* For ps_malloc, ps_realloc, free */
#define LV_MEM_CUSTOM_ALLOC      ps_malloc
#define LV_MEM_CUSTOM_FREE       free
#define LV_MEM_CUSTOM_REALLOC    ps_realloc
#define LV_MEM_SIZE         (64*1024U)    /* Ignored when LV_MEM_CUSTOM is 1 */

/* Screen resolution — portrait 240x320 */
#define LV_HOR_RES_MAX         240
#define LV_VER_RES_MAX         320

/* Tick — fed from millis() automatically */
#define LV_TICK_CUSTOM          1
#define LV_TICK_CUSTOM_INCLUDE  <Arduino.h>
#define LV_TICK_CUSTOM_SYS_TIME_EXPR  (millis())

/* DPI — 3.2 inch 240x320 ≈ 125 DPI */
#define LV_DPI_DEF            125

/* Debug logging — WARN normally; switch to INFO to see indev events */
#define LV_USE_PERF_MONITOR     0
#define LV_USE_LOG              1
#define LV_LOG_LEVEL            LV_LOG_LEVEL_WARN

/* Input device defaults — scroll needs deliberate 10px movement */
#define LV_INDEV_DEF_SCROLL_LIMIT       10
#define LV_INDEV_DEF_SCROLL_THROW       10
#define LV_INDEV_DEF_LONG_PRESS_TIME   400

/* Fonts */
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_18   1
#define LV_FONT_MONTSERRAT_20   1   /* settings row values, filter names */
#define LV_FONT_MONTSERRAT_24   1   /* panel titles */
#define LV_FONT_MONTSERRAT_36   1   /* countdown number */
#define LV_FONT_DEFAULT         &lv_font_montserrat_14

/* Widgets */
#define LV_USE_BTN              1
#define LV_USE_LABEL            1
#define LV_USE_IMG              1
#define LV_USE_CANVAS           1
#define LV_USE_LIST             1
#define LV_USE_SWITCH           1
#define LV_USE_SLIDER           0
#define LV_USE_ROLLER           0
#define LV_USE_DROPDOWN         0
#define LV_USE_TABLE            0
#define LV_USE_ANIMIMG          1
#define LV_USE_SNAPSHOT         1
#define LV_USE_MSGBOX           0
#define LV_USE_SPINBOX          0
#define LV_USE_CHART            0
#define LV_USE_METER            0

/* Layout — flex used for scrollable filter list */
#define LV_USE_FLEX             1
#define LV_USE_GRID             0

/* Additional core widgets — explicitly off */
#define LV_USE_TEXTAREA         0
#define LV_USE_ARC              1
#define LV_USE_CHECKBOX         0
#define LV_USE_BAR              0

/* Extra/compound widgets — explicitly disable all that depend on
   widgets we've turned off, or that we simply don't use */
#define LV_USE_CALENDAR                  0   /* needs LV_USE_DROPDOWN */
#define LV_USE_CALENDAR_HEADER_DROPDOWN  0   /* needs LV_USE_DROPDOWN */
#define LV_USE_CALENDAR_HEADER_ARROW     0
#define LV_USE_COLORWHEEL                0
#define LV_USE_LED                       0
#define LV_USE_KEYBOARD                  0   /* needs LV_USE_TEXTAREA */
#define LV_USE_TABVIEW                   0
#define LV_USE_TILEVIEW                  0
#define LV_USE_WIN                       0
#define LV_USE_SPAN                      0
#define LV_USE_IMGBTN                    0

/* Animation */
#define LV_USE_ANIMATION        1


#endif /*LV_CONF_H*/
#endif /*End of "Content enable"*/
