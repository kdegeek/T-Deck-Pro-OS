/**
 * @file lv_conf.h
 * @brief LVGL Configuration for T-Deck-Pro OS - UNIFIED VERSION
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Resolves conflicts between multiple LVGL config files
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COMPILER SETTINGS
 *====================*/

/*1: Enable the 32 bit CPU architecture*/
#define LV_USE_PERF_MONITOR 0

/*====================
   MEMORY SETTINGS
 *====================*/

/*CRITICAL FIX: Memory configuration optimized for ESP32-S3 with PSRAM*/
/*1: use custom malloc/free, 0: use the built-in `lv_mem_alloc()` and `lv_mem_free()`*/
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM == 0
    /*Size of the memory available for `lv_mem_alloc()` in bytes (>= 2kB)*/
    #define LV_MEM_SIZE (256 * 1024U)          /*256KB for LVGL - optimized for ESP32-S3*/
    /*Set an address for the memory pool instead of allocating it as a normal array. Can be in external SRAM too.*/
    #define LV_MEM_ADR 0     /*0: use malloc*/
    /*Instead of an address give a memory allocator that will be called to get a memory pool for LVGL. E.g. my_malloc*/
    #if LV_MEM_ADR == 0
        #undef LV_MEM_POOL_INCLUDE
        #undef LV_MEM_POOL_ALLOC
    #endif
#else       /*LV_MEM_CUSTOM*/
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>   /*Header for the dynamic memory function*/
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif     /*LV_MEM_CUSTOM*/

/*Number of the memory buffer*/
#define LV_MEM_BUF_MAX_NUM 16

/*Use the standard `memcpy` and `memset` instead of LVGL's own functions. (Might or might not be faster).*/
#define LV_MEMCPY_MEMSET_STD 1

/*====================
   HAL SETTINGS
 *====================*/

/*CRITICAL FIX: Tick configuration for ESP32*/
/*1: use a custom tick source that tells the elapsed time in milliseconds.
 *It removes the need to manually update the tick with `lv_tick_inc()`)*/
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"         /*Header for the system time function*/
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())    /*Expression evaluating to current system time in ms*/
#endif   /*LV_TICK_CUSTOM*/

/*Default display refresh period. LVG will redraw changed areas with this period time*/
#define LV_DISP_DEF_REFR_PERIOD 5000    /*5000ms for E-ink display (very slow refresh)*/

/*Input device read period in milliseconds*/
#define LV_INDEV_DEF_READ_PERIOD 30

/*Use a custom tick source that tells the elapsed time in milliseconds*/
#define LV_DPI_DEF 130     /*[px/inch]*/

/*====================
   COLOR SETTINGS
 *====================*/

/*CRITICAL FIX: Color depth for E-ink display*/
/*Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888)*/
#define LV_COLOR_DEPTH 1

#if LV_COLOR_DEPTH == 1
    /*0: RGB   1: BGR order*/
    #define LV_COLOR_16_SWAP 0
    /*Enable more complex drawing routines to manage screens transparency.
     *Can be used if the UI is above another layer, e.g. an OSD menu or video player.*/
    #define LV_COLOR_SCREEN_TRANSP 0
    /*Images pixels with this color will not be drawn if they are chroma keyed)*/
    #define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)         /*pure green*/
#endif

/*====================
   TEXT SETTINGS
 *====================*/

/*Select a character encoding for strings.
 *Your IDE or editor should have the same character encoding*/
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/*Can break (wrap) texts on these chars*/
#define LV_TXT_BREAK_CHARS " ,.;:-_"

/*If a word is at least this long, will break wherever "prettiest"
 *To disable, set to a value <= 0*/
#define LV_TXT_LINE_BREAK_LONG_LEN 0

/*Minimum number of characters in a long word to put on a line before a break.*/
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3

/*Minimum number of characters in a long word to put on a line after a break.*/
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/*The control character to use for signalling text recoloring.*/
#define LV_TXT_COLOR_CMD "#"

/*Support bidirectional texts. Allows mixing Left-to-Right and Right-to-Left texts.*/
#define LV_USE_BIDI 0
#if LV_USE_BIDI
    /*Set the default direction. Supported values:
    *`LV_BASE_DIR_LTR` Left-to-Right
    *`LV_BASE_DIR_RTL` Right-to-Left
    *`LV_BASE_DIR_AUTO` detect texts base direction*/
    #define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO
#endif

/*Enable Arabic/Persian processing*/
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*====================
   WIDGET USAGE
 *====================*/

/*Documentation of the widgets: https://docs.lvgl.io/latest/en/html/widgets/index.html*/

#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     0   /*Disabled for memory optimization*/
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1   /*Requires: lv_label*/
#define LV_USE_IMG        1   /*Requires: lv_label*/
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     1   /*Requires: lv_label*/
#define LV_USE_SLIDER     1   /*Requires: lv_bar*/
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   1   /*Requires: lv_label*/
#define LV_USE_TABLE      1   /*Requires: lv_label*/

/*==================
 * EXTRA COMPONENTS
 *==================*/

/*-----------
 * Widgets
 *----------*/
#define LV_USE_ANIMIMG    0   /*Disabled for memory*/
#define LV_USE_CALENDAR   0   /*Disabled for memory*/
#define LV_USE_CHART      0   /*Disabled for memory*/
#define LV_USE_COLORWHEEL 0   /*Disabled for memory*/
#define LV_USE_IMGBTN     1
#define LV_USE_KEYBOARD   1   /*Required for T-Deck-Pro keyboard*/
#define LV_USE_LED        1
#define LV_USE_LIST       1
#define LV_USE_MENU       1
#define LV_USE_METER      0   /*Disabled for memory*/
#define LV_USE_MSGBOX     1
#define LV_USE_SPINBOX    1
#define LV_USE_SPINNER    1
#define LV_USE_TABVIEW    1
#define LV_USE_TILEVIEW   1
#define LV_USE_WIN        1

/*-----------
 * Themes
 *----------*/

/*A simple, impressive and very complete theme*/
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    /*0: Light mode; 1: Dark mode*/
    #define LV_THEME_DEFAULT_DARK 0
    /*1: Enable grow on press*/
    #define LV_THEME_DEFAULT_GROW 1
    /*Default transition time in [ms]*/
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif /*LV_USE_THEME_DEFAULT*/

/*A very simple theme that is a good starting point for a custom theme*/
#define LV_USE_THEME_BASIC 1

/*A theme designed for monochrome displays*/
#define LV_USE_THEME_MONO 1

/*-----------
 * Layouts
 *----------*/

/*A layout similar to Flexbox in CSS.*/
#define LV_USE_FLEX 1

/*A layout similar to Grid in CSS.*/
#define LV_USE_GRID 1

/*====================
 * OTHERS
 *====================*/

/*1: Show CPU usage and FPS count*/
#define LV_USE_PERF_MONITOR 0   /*Disabled for production*/

/*1: Show the used memory and the memory fragmentation
 * Requires LV_MEM_CUSTOM = 0*/
#define LV_USE_MEM_MONITOR 0    /*Disabled for production*/

/*1: Draw random colored rectangles over the redrawn areas*/
#define LV_USE_REFR_DEBUG 0     /*Disabled for production*/

/*Change the built in (v)snprintf functions*/
#define LV_SPRINTF_CUSTOM 0
#if LV_SPRINTF_CUSTOM
    #define LV_SPRINTF_INCLUDE <stdio.h>
    #define lv_snprintf  snprintf
    #define lv_vsnprintf vsnprintf
#else   /*LV_SPRINTF_CUSTOM*/
    #define LV_SPRINTF_USE_FLOAT 0
#endif  /*LV_SPRINTF_CUSTOM*/

#define LV_USE_USER_DATA 1

/*Garbage Collector settings
 *Used if lvgl is bound to higher level language and the memory is managed by that language*/
#define LV_ENABLE_GC 0
#if LV_ENABLE_GC != 0
    #define LV_GC_INCLUDE "gc.h"                           /*Include Garbage Collector related things*/
#endif /*LV_ENABLE_GC*/

/*=====================
 *  COMPILER SETTINGS
 *====================*/

/*For big endian systems set to 1*/
#define LV_BIG_ENDIAN_SYSTEM 0

/*Define a custom attribute to `lv_tick_inc` function*/
#define LV_ATTRIBUTE_TICK_INC

/*Define a custom attribute to `lv_timer_handler` function*/
#define LV_ATTRIBUTE_TIMER_HANDLER

/*Define a custom attribute to `lv_disp_flush_ready` function*/
#define LV_ATTRIBUTE_FLUSH_READY

/*Required alignment size for buffers*/
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1

/*Will be added where memories needs to be aligned (with -Os data might not be aligned to boundary by default).
 * E.g. __attribute__((aligned(4)))*/
#define LV_ATTRIBUTE_MEM_ALIGN

/*Attribute to mark large constant arrays for example font's bitmaps*/
#define LV_ATTRIBUTE_LARGE_CONST

/*Complier prefix for a big array declaration in RAM*/
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY

/*Place performance critical functions into a faster memory (e.g RAM)*/
#define LV_ATTRIBUTE_FAST_MEM

/*Prefix variables that are used in GPU accelerated operations, often these need to be placed in RAM sections that are DMA accessible*/
#define LV_ATTRIBUTE_DMA

/*Export integer constant to binding. This macro is used with constants in the form of LV_<CONST> that
 *should also appear on LVGL binding API such as Micropython.*/
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning /*The default value just prevents GCC warning*/

/*Extend the default -32k..32k coordinate range to -4M..4M by using int32_t for coordinates instead of int16_t*/
#define LV_USE_LARGE_COORD 0

/*==================
 *   FONT USAGE
 *==================*/

/*Montserrat fonts with various styles and sizes*/
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 1   /*Required for UI*/
#define LV_FONT_MONTSERRAT_14 1   /*Required for UI*/
#define LV_FONT_MONTSERRAT_16 1   /*Required for UI*/
#define LV_FONT_MONTSERRAT_18 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_20 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_22 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_24 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_26 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_28 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_30 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_32 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_34 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_36 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_38 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_40 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_42 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_44 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_46 0   /*Memory optimization*/
#define LV_FONT_MONTSERRAT_48 0   /*Memory optimization*/

/*Demonstrate special features*/
#define LV_FONT_MONTSERRAT_12_SUBPX      0  /*Memory optimization*/
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0  /*bpp = 3*/
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0  /*Hebrew, Arabic, Perisan letters and all their forms*/
#define LV_FONT_SIMSUN_16_CJK            0  /*1000 most common CJK radicals*/

/*Pixel perfect monospace fonts*/
#define LV_FONT_UNSCII_8  1   /*Good for debug output*/
#define LV_FONT_UNSCII_16 0   /*Memory optimization*/

/*Optionally declare custom fonts here.*/

/*Always set a default font*/
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*Enable handling large font and/or fonts with a lot of characters.
 *The limit depends on the font size, font face and bpp.
 *Compiler error will be triggered if a font needs it.*/
#define LV_FONT_FMT_TXT_LARGE 0

/*Enables/disables support for compressed fonts.*/
#define LV_USE_FONT_COMPRESSED 0

/*Enable subpixel rendering*/
#define LV_USE_FONT_SUBPX 0
#if LV_USE_FONT_SUBPX
    /*Set the pixel order of the display. Physical order of RGB channels. Doesn't matter with "normal" fonts.*/
    #define LV_FONT_SUBPX_BGR 0  /*0: RGB; 1:BGR order*/
#endif

/*=================
 *  TEXT SETTINGS
 *================*/

/**
 * Select a character encoding for strings.
 * Your IDE or editor should have the same character encoding
 * - LV_TXT_ENC_UTF8
 * - LV_TXT_ENC_ASCII
 */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/*Can break (wrap) texts on these chars*/
#define LV_TXT_BREAK_CHARS " ,.;:-_"

/*If a word is at least this long, will break wherever "prettiest"
 *To disable, set to a value <= 0*/
#define LV_TXT_LINE_BREAK_LONG_LEN 0

/*Minimum number of characters in a long word to put on a line before a break.
 *Depends on LV_TXT_LINE_BREAK_LONG_LEN.*/
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3

/*Minimum number of characters in a long word to put on a line after a break.
 *Depends on LV_TXT_LINE_BREAK_LONG_LEN.*/
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/*The control character to use for signalling text recoloring.*/
#define LV_TXT_COLOR_CMD "#"

/*Support bidirectional texts. Allows mixing Left-to-Right and Right-to-Left texts.
 *The direction will be processed according to the Unicode Bidirectional Algorithm:
 *https://www.unicode.org/reports/tr9/*/
#define LV_USE_BIDI 0
#if LV_USE_BIDI
    /*Set the default direction. Supported values:
    *`LV_BASE_DIR_LTR` Left-to-Right
    *`LV_BASE_DIR_RTL` Right-to-Left
    *`LV_BASE_DIR_AUTO` detect texts base direction*/
    #define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO
#endif

/*Enable Arabic/Persian processing
 *In these languages characters should be replaced with an other form based on their position in the text*/
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*==================
 *  WIDGET USAGE
 *==================*/

/*Documentation of the widgets: https://docs.lvgl.io/latest/en/html/widgets/index.html*/

#define LV_USE_ARC        1
#define LV_USE_ANIMIMG    0
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     0

#define LV_USE_CHECKBOX   1

#define LV_USE_DROPDOWN   1   /*Requires: lv_label*/

#define LV_USE_IMG        1   /*Requires: lv_label*/

#define LV_USE_LABEL      1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION 1 /*Enable selecting text of the label*/
    #define LV_LABEL_LONG_TXT_HINT 1  /*Store some extra info in labels to speed up drawing of very long texts*/
#endif

#define LV_USE_LINE       1

#define LV_USE_ROLLER     1   /*Requires: lv_label*/
#if LV_USE_ROLLER
    #define LV_ROLLER_INF_PAGES 7 /*Number of extra "pages" when the roller is infinite*/
#endif

#define LV_USE_SLIDER     1   /*Requires: lv_bar*/

#define LV_USE_SWITCH     1

#define LV_USE_TEXTAREA   1   /*Requires: lv_label*/
#if LV_USE_TEXTAREA != 0
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500    /*ms*/
#endif

#define LV_USE_TABLE      1

/*==================
 * EXTRA COMPONENTS
 *==================*/

/*-----------
 * Widgets
 *----------*/
#define LV_USE_CALENDAR   0
#if LV_USE_CALENDAR
    #define LV_CALENDAR_WEEK_STARTS_MONDAY 0
    #if LV_CALENDAR_WEEK_STARTS_MONDAY
        #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"}
    #else
        #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"}
    #endif
    #define LV_CALENDAR_DEFAULT_MONTH_NAMES {"January", "February", "March",  "April", "May",  "June", "July", "August", "September", "October", "November", "December"}
    #define LV_USE_CALENDAR_HEADER_ARROW 1
    #define LV_USE_CALENDAR_HEADER_DROPDOWN 1
#endif  /*LV_USE_CALENDAR*/

#define LV_USE_CHART      0

#define LV_USE_COLORWHEEL 0

#define LV_USE_IMGBTN     1

#define LV_USE_KEYBOARD   1
#define LV_USE_LED        1
#define LV_USE_LIST       1
#define LV_USE_MENU       1
#define LV_USE_METER      0
#define LV_USE_MSGBOX     1
#define LV_USE_SPINBOX    1
#define LV_USE_SPINNER    1
#define LV_USE_TABVIEW    1
#define LV_USE_TILEVIEW   1
#define LV_USE_WIN        1

/*-----------
 * Themes
 *----------*/

/*A simple, impressive and very complete theme*/
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT

    /*0: Light mode; 1: Dark mode*/
    #define LV_THEME_DEFAULT_DARK 0

    /*1: Enable grow on press*/
    #define LV_THEME_DEFAULT_GROW 1

    /*Default transition time in [ms]*/
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif /*LV_USE_THEME_DEFAULT*/

/*A very simple theme that is a good starting point for a custom theme*/
#define LV_USE_THEME_BASIC 1

/*A theme designed for monochrome displays*/
#define LV_USE_THEME_MONO 1

/*-----------
 * Layouts
 *----------*/

/*A layout similar to Flexbox in CSS.*/
#define LV_USE_FLEX 1

/*A layout similar to Grid in CSS.*/
#define LV_USE_GRID 1

/*=====================
 *  DEVELOPER MODE
 *====================*/

/*Enable debug mode for T-Deck-Pro development*/
#ifdef DEBUG_MODE
    #define LV_USE_PERF_MONITOR 1
    #define LV_USE_MEM_MONITOR 1
    #define LV_USE_REFR_DEBUG 1
#endif

/*Enable assert in development*/
#define LV_USE_ASSERT_NULL 1         /*Check if the parameter is NULL. (Very fast, recommended)*/
#define LV_USE_ASSERT_MALLOC 1       /*Checks is the memory is successfully allocated or no. (Very fast, recommended)*/
#define LV_USE_ASSERT_STYLE 0        /*Check if the styles are properly initialized. (Very slow)*/
#define LV_USE_ASSERT_MEM_INTEGRITY 0 /*Check the integrity of `lv_mem` after critical operations. (Slow)*/
#define LV_USE_ASSERT_OBJ 0          /*Check the object's type and existence (e.g. not deleted). (Slow)*/

/*Add a custom handler when assert happens e.g. to restart the MCU*/
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);   /*Halt by default*/

/*-------------
 * Logging
 *-----------*/

/*1: Enable the log module*/
#define LV_USE_LOG 1
#if LV_USE_LOG

    /*How important log should be added:
    *LV_LOG_LEVEL_TRACE       A lot of logs to give detailed information
    *LV_LOG_LEVEL_INFO        Log important events
    *LV_LOG_LEVEL_WARN        Log if something unwanted happened but didn't cause a problem
    *LV_LOG_LEVEL_ERROR       Only critical issue, when the system may fail
    *LV_LOG_LEVEL_USER        Only logs added by the user
    *LV_LOG_LEVEL_NONE        Do not log anything*/
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

    /*1: Print the log with 'printf';
    *0: User need to register a callback with `lv_log_register_print_cb()`*/
    #define LV_LOG_PRINTF 1

    /*Enable/disable LV_LOG_TRACE in modules that produces a huge number of logs*/
    #define LV_LOG_TRACE_MEM        1
    #define LV_LOG_TRACE_TIMER      1
    #define LV_LOG_TRACE_INDEV      1
    #define LV_LOG_TRACE_DISP_REFR  1
    #define LV_LOG_TRACE_EVENT      1
    #define LV_LOG_TRACE_OBJ_CREATE 1
    #define LV_LOG_TRACE_LAYOUT     1
    #define LV_LOG_TRACE_ANIM       1

#endif  /*LV_USE_LOG*/

/*===================
 *  OTHERS
 *==================*/

/*1: Show CPU usage and FPS count in the right bottom corner*/
#define LV_USE_PERF_MONITOR 0

/*1: Use the functions and types from the older API if possible */
#define LV_USE_API_EXTENSION_V6 0
#define LV_USE_API_EXTENSION_V7 0

/*========================
 * EXAMPLES 
 *=======================*/

/*Enable the examples to be built with the library*/
#define LV_BUILD_EXAMPLES 0

/*===================
 *  DEMO USAGE
 *==================*/

/*Show some widget. It might be required to increase `LV_MEM_SIZE` */
#define LV_USE_DEMO_WIDGETS 0
#if LV_USE_DEMO_WIDGETS
    #define LV_DEMO_WIDGETS_SLIDESHOW 0
#endif

/*Demonstrate the usage of encoder and keyboard*/
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0

/*Benchmark your system*/
#define LV_USE_DEMO_BENCHMARK 0

/*Stress test for LVGL*/
#define LV_USE_DEMO_STRESS 0

/*Music player demo*/
#define LV_USE_DEMO_MUSIC 0
#if LV_USE_DEMO_MUSIC
    #define LV_DEMO_MUSIC_SQUARE    0
    #define LV_DEMO_MUSIC_LANDSCAPE 0
    #define LV_DEMO_MUSIC_ROUND     0
    #define LV_DEMO_MUSIC_LARGE     0
    #define LV_DEMO_MUSIC_AUTO_PLAY 0
#endif

/*--END OF LV_CONF_H--*/

#endif /*LV_CONF_H*/

#if LV_CONF_INCLUDE_SIMPLE == 1
    #include "lvgl.h"
#endif