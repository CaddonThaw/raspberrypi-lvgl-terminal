/**
 * @file ST7789.h
 *
 */

#ifndef ST7789_H
#define ST7789_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>
#ifndef LV_DRV_NO_CONF
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lv_drv_conf.h"
#else
#include "../../lv_drv_conf.h"
#endif
#endif

#if USE_ST7789

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#if LV_COLOR_DEPTH != 16
#error "ST7789 currently supports 'LV_COLOR_DEPTH == 16'. Set it in lv_conf.h"
#endif

#if LV_COLOR_16_SWAP != 1
#error "ST7789 SPI requires LV_COLOR_16_SWAP == 1. Set it in lv_conf.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define ST7789_BGR true
#define ST7789_RGB false

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void st7789_init(void);
void st7789_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p);
void st7789_rotate(int degrees, bool bgr);
/**********************
 *      MACROS
 **********************/

#endif /* USE_ST7789 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ST7789_H */