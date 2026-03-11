/**
 * @file ST7735S.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "ST7735S.h"
#if USE_ST7735S != 0

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include LV_DRV_DISP_INCLUDE
#include LV_DRV_DELAY_INCLUDE

/*********************
 *      DEFINES
 *********************/
#define ST7735S_CMD_MODE  0
#define ST7735S_DATA_MODE 1

#define ST7735S_TFTWIDTH  ST7735S_HOR_RES
#define ST7735S_TFTHEIGHT ST7735S_VER_RES

#define ST7735S_SWRESET   0x01
#define ST7735S_SLPOUT    0x11
#define ST7735S_NORON     0x13
#define ST7735S_INVOFF    0x20
#define ST7735S_DISPON    0x29
#define ST7735S_CASET     0x2A
#define ST7735S_RASET     0x2B
#define ST7735S_RAMWR     0x2C
#define ST7735S_MADCTL    0x36
#define ST7735S_COLMOD    0x3A

#define ST7735S_FRMCTR1   0xB1
#define ST7735S_FRMCTR2   0xB2
#define ST7735S_FRMCTR3   0xB3
#define ST7735S_INVCTR    0xB4
#define ST7735S_PWCTR1    0xC0
#define ST7735S_PWCTR2    0xC1
#define ST7735S_PWCTR3    0xC2
#define ST7735S_PWCTR4    0xC3
#define ST7735S_PWCTR5    0xC4
#define ST7735S_VMCTR1    0xC5
#define ST7735S_GMCTRP1   0xE0
#define ST7735S_GMCTRN1   0xE1

#define MADCTL_MY         0x80
#define MADCTL_MX         0x40
#define MADCTL_MV         0x20
#define MADCTL_RGB        0x00
#define MADCTL_BGR        0x08

#ifndef ST7735S_XSTART
#define ST7735S_XSTART 0
#endif

#ifndef ST7735S_YSTART
#define ST7735S_YSTART 0
#endif

/**********************
 *  STATIC PROTOTYPES
 **********************/
static inline void st7735s_write(int mode, uint8_t data);
static inline void st7735s_write_array(int mode, uint8_t *data, uint16_t len);
static inline void st7735s_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**********************
 *   STATIC FUNCTIONS
 **********************/
static inline void st7735s_write(int mode, uint8_t data)
{
    digitalWrite(PIN_CS, 0);
    digitalWrite(PIN_DC, mode);
    wiringPiSPIDataRW(SPI_CHANNEL, &data, 1);
    digitalWrite(PIN_CS, 1);
}

static inline void st7735s_write_array(int mode, uint8_t *data, uint16_t len)
{
    digitalWrite(PIN_CS, 0);
    digitalWrite(PIN_DC, mode);
    wiringPiSPIDataRW(SPI_CHANNEL, data, len);
    digitalWrite(PIN_CS, 1);
}

static inline void st7735s_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t data[4];

    x0 += ST7735S_XSTART;
    x1 += ST7735S_XSTART;
    y0 += ST7735S_YSTART;
    y1 += ST7735S_YSTART;

    st7735s_write(ST7735S_CMD_MODE, ST7735S_CASET);
    data[0] = (uint8_t)(x0 >> 8);
    data[1] = (uint8_t)(x0 & 0xFFU);
    data[2] = (uint8_t)(x1 >> 8);
    data[3] = (uint8_t)(x1 & 0xFFU);
    st7735s_write_array(ST7735S_DATA_MODE, data, 4);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_RASET);
    data[0] = (uint8_t)(y0 >> 8);
    data[1] = (uint8_t)(y0 & 0xFFU);
    data[2] = (uint8_t)(y1 >> 8);
    data[3] = (uint8_t)(y1 & 0xFFU);
    st7735s_write_array(ST7735S_DATA_MODE, data, 4);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_RAMWR);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void st7735s_init(void)
{
    uint8_t data[16];

    if(wiringPiSetupGpio() == -1) {
        fprintf(stderr, "Init WiringPi fail\n");
        exit(EXIT_FAILURE);
    }

    if(wiringPiSPISetupMode(SPI_CHANNEL, SPI_SPEED, 0) < 0) {
        fprintf(stderr, "Init SPI fail\n");
        exit(EXIT_FAILURE);
    }

    pinMode(PIN_CS, OUTPUT);
    pinMode(PIN_DC, OUTPUT);
    pinMode(PIN_RST, OUTPUT);
    pinMode(PIN_BLK, OUTPUT);

    digitalWrite(PIN_CS, 1);
    digitalWrite(PIN_DC, ST7735S_DATA_MODE);
    digitalWrite(PIN_RST, 0);
    usleep(50000);
    digitalWrite(PIN_RST, 1);
    usleep(120000);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_SWRESET);
    usleep(150000);
    st7735s_write(ST7735S_CMD_MODE, ST7735S_SLPOUT);
    usleep(120000);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_FRMCTR1);
    data[0] = 0x01; data[1] = 0x2C; data[2] = 0x2D;
    st7735s_write_array(ST7735S_DATA_MODE, data, 3);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_FRMCTR2);
    data[0] = 0x01; data[1] = 0x2C; data[2] = 0x2D;
    st7735s_write_array(ST7735S_DATA_MODE, data, 3);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_FRMCTR3);
    data[0] = 0x01; data[1] = 0x2C; data[2] = 0x2D;
    data[3] = 0x01; data[4] = 0x2C; data[5] = 0x2D;
    st7735s_write_array(ST7735S_DATA_MODE, data, 6);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_INVCTR);
    data[0] = 0x07;
    st7735s_write_array(ST7735S_DATA_MODE, data, 1);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_PWCTR1);
    data[0] = 0xA2; data[1] = 0x02; data[2] = 0x84;
    st7735s_write_array(ST7735S_DATA_MODE, data, 3);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_PWCTR2);
    data[0] = 0xC5;
    st7735s_write_array(ST7735S_DATA_MODE, data, 1);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_PWCTR3);
    data[0] = 0x0A; data[1] = 0x00;
    st7735s_write_array(ST7735S_DATA_MODE, data, 2);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_PWCTR4);
    data[0] = 0x8A; data[1] = 0x2A;
    st7735s_write_array(ST7735S_DATA_MODE, data, 2);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_PWCTR5);
    data[0] = 0x8A; data[1] = 0xEE;
    st7735s_write_array(ST7735S_DATA_MODE, data, 2);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_VMCTR1);
    data[0] = 0x0E;
    st7735s_write_array(ST7735S_DATA_MODE, data, 1);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_INVOFF);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_COLMOD);
    st7735s_write(ST7735S_DATA_MODE, 0x05);

    st7735s_rotate(ST7735S_ROTATION, ST7735S_BGR_ORDER ? ST7735S_BGR : ST7735S_RGB);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_GMCTRP1);
    data[0] = 0x02; data[1] = 0x1C; data[2] = 0x07; data[3] = 0x12;
    data[4] = 0x37; data[5] = 0x32; data[6] = 0x29; data[7] = 0x2D;
    data[8] = 0x29; data[9] = 0x25; data[10] = 0x2B; data[11] = 0x39;
    data[12] = 0x00; data[13] = 0x01; data[14] = 0x03; data[15] = 0x10;
    st7735s_write_array(ST7735S_DATA_MODE, data, 16);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_GMCTRN1);
    data[0] = 0x03; data[1] = 0x1D; data[2] = 0x07; data[3] = 0x06;
    data[4] = 0x2E; data[5] = 0x2C; data[6] = 0x29; data[7] = 0x2D;
    data[8] = 0x2E; data[9] = 0x2E; data[10] = 0x37; data[11] = 0x3F;
    data[12] = 0x00; data[13] = 0x00; data[14] = 0x02; data[15] = 0x10;
    st7735s_write_array(ST7735S_DATA_MODE, data, 16);

    st7735s_write(ST7735S_CMD_MODE, ST7735S_NORON);
    usleep(10000);

    st7735s_set_window(0, 0, ST7735S_TFTWIDTH - 1, ST7735S_TFTHEIGHT - 1);
    st7735s_write(ST7735S_CMD_MODE, ST7735S_DISPON);
    usleep(100000);

    digitalWrite(PIN_BLK, 1);
}

void st7735s_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
    if(area->x2 < 0 || area->y2 < 0 || area->x1 > (ST7735S_TFTWIDTH - 1) || area->y1 > (ST7735S_TFTHEIGHT - 1)) {
        lv_disp_flush_ready(drv);
        return;
    }

    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 = area->x2 > ST7735S_TFTWIDTH - 1 ? ST7735S_TFTWIDTH - 1 : area->x2;
    int32_t act_y2 = area->y2 > ST7735S_TFTHEIGHT - 1 ? ST7735S_TFTHEIGHT - 1 : area->y2;

    int32_t y;
    int32_t len = (act_x2 - act_x1 + 1) * 2;
    lv_coord_t w = (area->x2 - area->x1) + 1;

    digitalWrite(PIN_CS, 0);
    st7735s_set_window((uint16_t)act_x1, (uint16_t)act_y1, (uint16_t)act_x2, (uint16_t)act_y2);

    digitalWrite(PIN_DC, ST7735S_DATA_MODE);
    for(y = act_y1; y <= act_y2; y++) {
        wiringPiSPIDataRW(SPI_CHANNEL, (uint8_t *)color_p, len);
        color_p += w;
    }

    digitalWrite(PIN_CS, 1);
    lv_disp_flush_ready(drv);
}

void st7735s_rotate(int degrees, bool bgr)
{
    uint8_t color_order = bgr ? MADCTL_BGR : MADCTL_RGB;

    st7735s_write(ST7735S_CMD_MODE, ST7735S_MADCTL);

    switch(degrees) {
    case 90:
        st7735s_write(ST7735S_DATA_MODE, MADCTL_MY | MADCTL_MV | color_order);
        break;
    case 180:
        st7735s_write(ST7735S_DATA_MODE, color_order);
        break;
    case 270:
        st7735s_write(ST7735S_DATA_MODE, MADCTL_MX | MADCTL_MV | color_order);
        break;
    case 0:
    default:
        st7735s_write(ST7735S_DATA_MODE, MADCTL_MX | MADCTL_MY | color_order);
        break;
    }
}

#endif
