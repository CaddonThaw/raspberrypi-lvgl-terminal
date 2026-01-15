/**
 * @file ST7789.c
 *
 * ST7789.pdf [ST7789V datasheet, version 1.0]
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "ST7789.h"
#if USE_ST7789 != 0

#include <stdio.h>
#include <stdbool.h>
#include LV_DRV_DISP_INCLUDE
#include LV_DRV_DELAY_INCLUDE
#include <wiringPi.h>                   //wiringPi library
#include <wiringPiSPI.h>                //wiringPi SPI library
#include <unistd.h>                     //usleep function
#include <stdlib.h>                     //malloc function

/*********************
 *      DEFINES
 *********************/
#define ST7789_CMD_MODE    0
#define ST7789_DATA_MODE   1

#define ST7789_TFTWIDTH    ST7789_HOR_RES
#define ST7789_TFTHEIGHT   ST7789_VER_RES

/* ST7789 Commands */
#define ST7789_NOP         0x00    /* No Operation */
#define ST7789_SWRESET     0x01    /* Software Reset */
#define ST7789_RDDID       0x04    /* Read Display ID */
#define ST7789_RDDST       0x09    /* Read Display Status */
#define ST7789_RDDPM       0x0A    /* Read Display Power Mode */
#define ST7789_RDDMADCTL   0x0B    /* Read Display MADCTL */
#define ST7789_RDDCOLMOD   0x0C    /* Read Display Pixel Format */
#define ST7789_RDDDIM      0x0D    /* Read Display Image Mode */
#define ST7789_RDDSM        0x0E    /* Read Display Signal Mode */
#define ST7789_RDDSDR       0x0F    /* Read Display Self-Diagnostic Result */
#define ST7789_SLPIN        0x10    /* Enter Sleep Mode */
#define ST7789_SLPOUT       0x11    /* Exit Sleep Mode */
#define ST7789_PTLON        0x12    /* Partial Display Mode ON */
#define ST7789_NORON        0x13    /* Normal Display Mode ON */
#define ST7789_INVOFF       0x20    /* Display Inversion OFF */
#define ST7789_INVON        0x21    /* Display Inversion ON */
#define ST7789_GAMSET       0x26    /* Gamma Set */
#define ST7789_DISPOFF      0x28    /* Display OFF */
#define ST7789_DISPON       0x29    /* Display ON */
#define ST7789_CASET        0x2A    /* Column Address Set */
#define ST7789_RASET        0x2B    /* Row Address Set */
#define ST7789_RAMWR        0x2C    /* Memory Write */
#define ST7789_RAMRD        0x2E    /* Memory Read */
#define ST7789_PTLAR        0x30    /* Partial Area */
#define ST7789_VSCRDEF      0x33    /* Vertical Scrolling Definition */
#define ST7789_TEOFF        0x34    /* Tearing Effect Line OFF */
#define ST7789_TEON         0x35    /* Tearing Effect Line ON */
#define ST7789_MADCTL       0x36    /* Memory Data Access Control */
#define     MADCTL_MY       0x80    /* Row Address Order */
#define     MADCTL_MX       0x40    /* Column Address Order */
#define     MADCTL_MV       0x20    /* Row/Column Exchange */
#define     MADCTL_ML       0x10    /* Vertical Refresh Order */
#define     MADCTL_MH       0x04    /* Horizontal Refresh Order */
#define     MADCTL_RGB      0x00    /* RGB Order */
#define     MADCTL_BGR      0x08    /* BGR Order */
#define ST7789_VSCRSADD     0x37    /* Vertical Scrolling Start Address */
#define ST7789_IDMOFF       0x38    /* Idle Mode OFF */
#define ST7789_IDMON        0x39    /* Idle Mode ON */
#define ST7789_COLMOD       0x3A    /* Interface Pixel Format - defines GRAM data bus width and LCD module interface color format */
#define ST7789_RDMODE       0x0A
#define ST7789_RDMADCTL     0x0B
#define ST7789_RDPIXFMT     0x0C
#define ST7789_RDIMGFMT     0x0D
#define ST7789_RDSELFDIAG   0x0F

/* Extended Commands */
#define ST7789_WRDISBV      0x51    /* Write Display Brightness */
#define ST7789_RDDISBV      0x52    /* Read Display Brightness */
#define ST7789_WRCTRLD      0x53    /* Write CTRL Display */
#define ST7789_RDCTRLD      0x54    /* Read CTRL Display */
#define ST7789_WRCACE       0x55    /* Write Content Adaptive Brightness Control and Color Enhancement */
#define ST7789_RDCACE       0x56    /* Read Content Adaptive Brightness Control */
#define ST7789_WRCABCMBREQ  0x5E    /* Write CABC Minimum Brightness */
#define ST7789_RDCABCMBREQ  0x5F    /* Read CABC Minimum Brightness */

/* Frame Rate Control */
#define ST7789_FRMCTR1      0xB1    /* In Normal Mode (Full Colors) */
#define ST7789_FRMCTR2      0xB2    /* In Idle Mode (8-colors) */
#define ST7789_FRMCTR3      0xB3    /* In Partial Mode (Full Colors) */
#define ST7789_INVCTR       0xB4    /* Display Inversion Control */
#define ST7789_DFUNCTR      0xB6    /* Display Function Control */

/* Power Control */
#define ST7789_PWCTR1       0xC0    /* Power Control 1 */
#define ST7789_PWCTR2       0xC1    /* Power Control 2 */
#define ST7789_PWCTR3       0xC2    /* Power Control 3 in Normal Mode */
#define ST7789_PWCTR4       0xC3    /* Power Control 4 in Idle Mode */
#define ST7789_PWCTR5       0xC4    /* Power Control 5 in Partial Mode */
#define ST7789_VMCTR1       0xC5    /* VCOM Control 1 */
#define ST7789_VMCTR2       0xC7    /* VCOM Control 2 */

/* Gamma Control */
#define ST7789_GMCTRP1      0xE0    /* Gamma (+) Correction Characteristics Setting */
#define ST7789_GMCTRN1      0xE1    /* Gamma (-) Correction Characteristics Setting */

/**********************
 *  STATIC PROTOTYPES
 **********************/
static inline void st7789_write(int mode, uint8_t data);
static inline void st7789_write_array(int mode, uint8_t *data, uint16_t len);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void st7789_send_byte(uint8_t data)
{
    wiringPiSPIDataRW(SPI_CHANNEL, &data, 1);
}

/**
 * Initialize the ST7789 display controller
 */
void st7789_init(void)
{
    uint8_t data[15];

    /* init wiringPi */
    if (wiringPiSetupGpio() == -1) {  
        fprintf(stderr, "Init WiringPi fail\n");
        exit(EXIT_FAILURE);
    }

    /* init SPI */ 
    int spi_fd = wiringPiSPISetupMode(SPI_CHANNEL, SPI_SPEED, 0); // Mode 0
    if (spi_fd < 0) {
        fprintf(stderr, "Init SPI fail\n");
        exit(EXIT_FAILURE);
    }

    /* set pin mode */
    pinMode(PIN_CS, OUTPUT);
    pinMode(PIN_DC, OUTPUT);
    pinMode(PIN_RST, OUTPUT);
    pinMode(PIN_BLK, OUTPUT);

    /* hardware reset */
    digitalWrite(PIN_CS, 1); 
    digitalWrite(PIN_DC, ST7789_DATA_MODE); 
    digitalWrite(PIN_RST, 0); 
    usleep(50000);  // Wait at least 10ms for reset
    digitalWrite(PIN_RST, 1); 
    usleep(120000); // Wait at least 120ms after reset

    /* software reset */
    st7789_write(ST7789_CMD_MODE, ST7789_SWRESET);
    usleep(150000); // Wait at least 150ms after software reset

    /* exit sleep mode */
    st7789_write(ST7789_CMD_MODE, ST7789_SLPOUT);
    usleep(120000); // Wait at least 120ms to exit sleep mode

    /* set pixel format to 16-bit color (RGB565) */
    st7789_write(ST7789_CMD_MODE, ST7789_COLMOD);
    st7789_write(ST7789_DATA_MODE, 0x05); // 16-bit color RGB565

    /* set display direction */
    st7789_rotate(270, ST7789_RGB);

    /* frame rate control */
    st7789_write(ST7789_CMD_MODE, ST7789_FRMCTR1);
    data[0] = 0x0C;  // RTNA [3:0] Front porch setting in normal mode
    data[1] = 0x0C;  // RTNB [3:0] Back porch setting in normal mode
    data[2] = 0x00;  // DIVA [2:0] Oscillator frequency division ratio in normal mode
    data[3] = 0x33;  // DIVB [3:0] Division ratio setting in normal mode
    data[4] = 0x33;  // DIVB [3:0] Division ratio setting in normal mode
    st7789_write_array(ST7789_DATA_MODE, data, 5);

    /* porch control */
    st7789_write(ST7789_CMD_MODE, 0xB2); // Porch Setting
    data[0] = 0x0C;
    data[1] = 0x0C;
    data[2] = 0x00;
    data[3] = 0x33;
    data[4] = 0x33;
    st7789_write_array(ST7789_DATA_MODE, data, 5);

    /* gate control */
    st7789_write(ST7789_CMD_MODE, 0xB7); // Gate Control
    data[0] = 0x35; // Default value
    st7789_write_array(ST7789_DATA_MODE, data, 1);

    /* VCOM setting */
    st7789_write(ST7789_CMD_MODE, 0xBB); // VCOM setting
    data[0] = 0x19; // VCOM_DUMMY [7:0] VCOM amplitude setting
    st7789_write_array(ST7789_DATA_MODE, data, 1);

    /* LCM setting */
    st7789_write(ST7789_CMD_MODE, 0xC0); // LCM Control
    data[0] = 0x2C; // LCMDRV [7:4], GD [3], SM [2], SS [1], ISC [0]
    st7789_write_array(ST7789_DATA_MODE, data, 1);

    /* VDV and VRH command enable */
    st7789_write(ST7789_CMD_MODE, 0xC2); // VDV and VRH command enable
    data[0] = 0x01; // VRH_EN [0]
    st7789_write_array(ST7789_DATA_MODE, data, 1);

    /* VRH setting */
    st7789_write(ST7789_CMD_MODE, 0xC3); // VRH Set
    data[0] = 0x12; // VRH [7:0] AVDD voltage setting
    st7789_write_array(ST7789_DATA_MODE, data, 1);

    /* VDV setting */
    st7789_write(ST7789_CMD_MODE, 0xC4); // VDV Set
    data[0] = 0x20; // VDV [6:0] VCOM alternating amplitude setting
    st7789_write_array(ST7789_DATA_MODE, data, 1);

    /* frame rate control */
    st7789_write(ST7789_CMD_MODE, 0xC6); // Frame Rate Control in Normal Mode
    data[0] = 0x0F; // FR_CTRL [3:0] Division ratio of internal clock
    st7789_write_array(ST7789_DATA_MODE, data, 1);

    /* power control 1 */
    st7789_write(ST7789_CMD_MODE, ST7789_PWCTR1);
    data[0] = 0xA4; // AVDD [7:4], AVCL [3:0]
    data[1] = 0xA1; // VGH [7:4], VGL [3:0]
    st7789_write_array(ST7789_DATA_MODE, data, 2);

    /* positive gamma correction */
    st7789_write(ST7789_CMD_MODE, ST7789_GMCTRP1);
    data[0] = 0xD0;
    data[1] = 0x04;
    data[2] = 0x0D;
    data[3] = 0x11;
    data[4] = 0x13;
    data[5] = 0x2B;
    data[6] = 0x3F;
    data[7] = 0x54;
    data[8] = 0x4C;
    data[9] = 0x18;
    data[10] = 0x0D;
    data[11] = 0x0B;
    data[12] = 0x1F;
    data[13] = 0x23;
    st7789_write_array(ST7789_DATA_MODE, data, 14);

    /* negative gamma correction */
    st7789_write(ST7789_CMD_MODE, ST7789_GMCTRN1);
    data[0] = 0xD0;
    data[1] = 0x04;
    data[2] = 0x0C;
    data[3] = 0x11;
    data[4] = 0x13;
    data[5] = 0x2C;
    data[6] = 0x3F;
    data[7] = 0x44;
    data[8] = 0x51;
    data[9] = 0x2F;
    data[10] = 0x1F;
    data[11] = 0x1F;
    data[12] = 0x20;
    data[13] = 0x23;
    st7789_write_array(ST7789_DATA_MODE, data, 14);

    /* display inversion off */
    st7789_write(ST7789_CMD_MODE, ST7789_INVOFF);

    /* set column address */
    st7789_write(ST7789_CMD_MODE, ST7789_CASET);
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = (ST7789_TFTWIDTH - 1) >> 8;
    data[3] = (ST7789_TFTWIDTH - 1);
    st7789_write_array(ST7789_DATA_MODE, data, 4);

    /* set row address */
    st7789_write(ST7789_CMD_MODE, ST7789_RASET);
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = (ST7789_TFTHEIGHT - 1) >> 8;
    data[3] = (ST7789_TFTHEIGHT - 1);
    st7789_write_array(ST7789_DATA_MODE, data, 4);

    /* enable display */
    st7789_write(ST7789_CMD_MODE, ST7789_DISPON);
    
    /* turn on backlight */
    digitalWrite(PIN_BLK, 1);
    usleep(20000);
}

void st7789_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
    if(area->x2 < 0 || area->y2 < 0 || area->x1 > (ST7789_TFTWIDTH - 1) || area->y1 > (ST7789_TFTHEIGHT - 1)) {
        lv_disp_flush_ready(drv);
        return;
    }

    /* Truncate the area to the screen */
    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 = area->x2 > ST7789_TFTWIDTH - 1 ? ST7789_TFTWIDTH - 1 : area->x2;
    int32_t act_y2 = area->y2 > ST7789_TFTHEIGHT - 1 ? ST7789_TFTHEIGHT - 1 : area->y2;

    int32_t y;
    uint8_t data[4];
    int32_t len = (act_x2 - act_x1 + 1) * 2;
    lv_coord_t w = (area->x2 - area->x1) + 1;

    digitalWrite(PIN_CS, 0);   // CS low

    /* window horizontal */
    digitalWrite(PIN_DC, ST7789_CMD_MODE);  
    st7789_send_byte(ST7789_CASET);
    data[0] = act_x1 >> 8;
    data[1] = act_x1;
    data[2] = act_x2 >> 8;
    data[3] = act_x2;
    digitalWrite(PIN_DC, ST7789_DATA_MODE);  
    wiringPiSPIDataRW(SPI_CHANNEL, data, 4);

    /* window vertical */
    digitalWrite(PIN_DC, ST7789_CMD_MODE);  
    st7789_send_byte(ST7789_RASET);
    data[0] = act_y1 >> 8;
    data[1] = act_y1;
    data[2] = act_y2 >> 8;
    data[3] = act_y2;
    digitalWrite(PIN_DC, ST7789_DATA_MODE);  
    wiringPiSPIDataRW(SPI_CHANNEL, data, 4);

    digitalWrite(PIN_DC, ST7789_CMD_MODE);
    st7789_send_byte(ST7789_RAMWR);

    digitalWrite(PIN_DC, ST7789_DATA_MODE);
    for(y = act_y1; y <= act_y2; y++) {
        wiringPiSPIDataRW(SPI_CHANNEL, (uint8_t *)color_p, len);
        color_p += w;
    }

    digitalWrite(PIN_CS, 1);   // CS high

    lv_disp_flush_ready(drv);
}

void st7789_rotate(int degrees, bool bgr)
{
    uint8_t color_order = MADCTL_RGB;

    if(bgr)
        color_order = MADCTL_BGR;

    st7789_write(ST7789_CMD_MODE, ST7789_MADCTL);

    switch(degrees) {
    case 270:
        st7789_write(ST7789_DATA_MODE, MADCTL_MV | color_order);
        break;
    case 180:
        st7789_write(ST7789_DATA_MODE, MADCTL_MY | color_order);
        break;
    case 90:
        st7789_write(ST7789_DATA_MODE, MADCTL_MX | MADCTL_MY | MADCTL_MV | color_order);
        break;
    case 0:
        /* fall-through */
    default:
        st7789_write(ST7789_DATA_MODE, MADCTL_MX | color_order);
        break;
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Write byte
 * @param mode sets command or data mode for write
 * @param byte the byte to write
 */
static inline void st7789_write(int mode, uint8_t data)
{
    digitalWrite(PIN_CS, 0);     // CS low
    digitalWrite(PIN_DC, mode);  // DC high
    wiringPiSPIDataRW(SPI_CHANNEL, &data, 1);
    digitalWrite(PIN_CS, 1);     // CS high
}

/**
 * Write byte array
 * @param mode sets command or data mode for write
 * @param data the byte array to write
 * @param len the length of the byte array
 */
static inline void st7789_write_array(int mode, uint8_t *data, uint16_t len)
{
    digitalWrite(PIN_CS, 0);   // CS low
    digitalWrite(PIN_DC, mode);  
    wiringPiSPIDataRW(SPI_CHANNEL, data, len);
    digitalWrite(PIN_CS, 1);   // CS high
}

#endif