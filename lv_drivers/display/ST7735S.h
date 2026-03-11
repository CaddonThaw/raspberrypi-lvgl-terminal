/**
 * @file ST7735S.h
 */

#ifndef ST7735S_H
#define ST7735S_H

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

#if USE_ST7735S

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#if LV_COLOR_DEPTH != 16
#error "ST7735S currently supports 'LV_COLOR_DEPTH == 16'. Set it in lv_conf.h"
#endif

#if LV_COLOR_16_SWAP != 1
#error "ST7735S SPI requires LV_COLOR_16_SWAP == 1. Set it in lv_conf.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define ST7735S_BGR true
#define ST7735S_RGB false

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void st7735s_init(void);
void st7735s_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p);
void st7735s_rotate(int degrees, bool bgr);

#endif /* USE_ST7735S */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ST7735S_H */
