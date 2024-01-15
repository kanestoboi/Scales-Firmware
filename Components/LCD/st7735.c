/**
 * Copyright (c) 2017 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "sdk_common.h"

#ifndef ST7735_DC_PIN
#define ST7735_DC_PIN 16
#endif

#ifndef ST7735_HEIGHT
#define ST7735_HEIGHT 160
#endif


#ifndef ST7735_WIDTH
#define ST7735_WIDTH 80
#endif


#ifndef ST7735_SCK_PIN
#define ST7735_SCK_PIN 14
#endif

// <o> ST7735_MISO_PIN - Pin number  <0-47> 


#ifndef ST7735_MISO_PIN
#define ST7735_MISO_PIN 12
#endif

// <o> ST7735_MOSI_PIN - Pin number  <0-47> 


#ifndef ST7735_MOSI_PIN
#define ST7735_MOSI_PIN 12
#endif

// <o> ST7735_SS_PIN - Pin number  <0-47> 


#ifndef ST7735_SS_PIN
#define ST7735_SS_PIN 17
#endif

#ifndef ST7735_TAB_COLOR
#define ST7735_TAB_COLOR INITR_BLUETAB
#endif

#if NRF_MODULE_ENABLED(ST7735)

#include "nrf_lcd.h"
#include "nrfx_spi.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"

// Set of commands described in ST7735 data sheet.
#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E

#define ST7735_PTLAR   0x30
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

#define ST7735_MADCTL_MY  0x80
#define ST7735_MADCTL_MX  0x40
#define ST7735_MADCTL_MV  0x20
#define ST7735_MADCTL_ML  0x10
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH  0x04
/* @} */

#define RGB2BGR(x)      (x << 11) | (x & 0x07E0) | (x >> 11)

static const nrfx_spi_t spi = NRFX_SPI_INSTANCE(ST7735_SPI_INSTANCE);  /**< SPI instance. */

/**
 * @brief Structure holding ST7735 controller basic parameters.
 */
typedef struct
{
    uint8_t tab_color;      /**< Color of tab attached to the used screen. */
}st7735_t;

/**
 * @brief Enumerator with TFT tab colors.
 */
typedef enum{
    INITR_GREENTAB = 0,     /**< Green tab. */
    INITR_REDTAB,           /**< Red tab. */
    INITR_BLACKTAB,         /**< Black tab. */
    INITR_144GREENTAB,       /**< Green tab, 1.44" display. */
    INITR_BLUETAB
}st7735_tab_t;

static st7735_t m_st7735;


static inline void spi_write(const void * data, size_t size)
{
    nrfx_spi_xfer_desc_t desc;

    desc.p_tx_buffer = data;
    desc.tx_length = size;
    desc.p_rx_buffer = NULL;
    desc.rx_length = 0;

    APP_ERROR_CHECK(nrfx_spi_xfer(&spi, &desc, 0));
}

static inline void write_data_buffered(uint8_t * c, uint16_t len)
{
    nrf_gpio_pin_set(ST7735_DC_PIN);

    spi_write(c, len);
}

static inline void write_command(uint8_t c)
{
    nrf_gpio_pin_clear(ST7735_DC_PIN);
    spi_write(&c, sizeof(c));
}

static inline void write_data(uint8_t c)
{
    nrf_gpio_pin_set(ST7735_DC_PIN);
    spi_write(&c, sizeof(c));
}

static void set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    ASSERT(x0 <= x1);
    ASSERT(y0 <= y1);

    x0 += 1;  // Add horizontal offset
    y0 += 26; // Add vertical offset
    x1 += 1;
    y1 += 26;
    
    write_command(ST7735_CASET);
    write_data(0x00);                       // For a 128x160 display, it is always 0.
    write_data(x0);
    write_data(0x00);                       // For a 128x160 display, it is always 0.
    write_data(x1);
    write_command(ST7735_RASET);
    write_data(0x00);                       // For a 128x160 display, it is always 0.
    write_data(y0);
    write_data(0x00);                       // For a 128x160 display, it is always 0.
    write_data(y1);
    write_command(ST7735_RAMWR);
}

static void command_list(void)
{
    write_command(ST7735_SWRESET);
    nrf_delay_ms(150);
    write_command(ST7735_SLPOUT);
    nrf_delay_ms(500);

    write_command(ST7735_FRMCTR1);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    write_command(ST7735_FRMCTR2);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    write_command(ST7735_FRMCTR3);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);

    write_command(ST7735_INVCTR);
    write_data(0x07);

    write_command(ST7735_PWCTR1);
    write_data(0xA2);
    write_data(0x02);
    write_data(0x84);
    write_command(ST7735_PWCTR2);
    write_data(0xC5);
    write_command(ST7735_PWCTR3);
    write_data(0x0A);
    write_data(0x00);
    write_command(ST7735_PWCTR3);
    write_data(0x8A);
    write_data(0x2A);
    write_command(ST7735_PWCTR5);
    write_data(0x8A);
    write_data(0xEE);
    write_data(0x0E);

    write_command(ST7735_INVOFF);
    write_command(ST7735_MADCTL);
    write_data(0xC8);

    write_command(ST7735_COLMOD);
    write_data(0x05);

    if (m_st7735.tab_color == INITR_GREENTAB)
    {
        write_command(ST7735_CASET);
        write_data(0x00);
        write_data(0x02);
        write_data(0x00);
        write_data(0x81);
        write_command(ST7735_RASET);
        write_data(0x00);
        write_data(0x01);
        write_data(0x00);
        write_data(0xA0);
    }
    else if (m_st7735.tab_color == INITR_144GREENTAB)
    {
        write_command(ST7735_CASET);
        write_data(0x00);
        write_data(0x00);
        write_data(0x00);
        write_data(0x7F);
        write_command(ST7735_RASET);
        write_data(0x00);
        write_data(0x00);
        write_data(0x00);
        write_data(0x7F);
    }
    else if (m_st7735.tab_color == INITR_REDTAB)
    {
        write_command(ST7735_CASET);
        write_data(0x00);
        write_data(0x00);
        write_data(0x00);
        write_data(0x7F);
        write_command(ST7735_RASET);
        write_data(0x00);
        write_data(0x00);
        write_data(0x00);
        write_data(0x9F);
    }
    else if (m_st7735.tab_color == INITR_BLUETAB)
    {
        write_command(ST7735_CASET);
        write_data(0x00);
        write_data(0x02);
        write_data(0x00);
        write_data(0x00);
        write_command(ST7735_RASET);
        write_data(0x00);
        write_data(0x1B);
        write_data(0x00);
        write_data(0xBB);
    }

    write_command(ST7735_GMCTRP1);
    write_data(0x02);
    write_data(0x1c);
    write_data(0x07);
    write_data(0x12);
    write_data(0x37);
    write_data(0x32);
    write_data(0x29);
    write_data(0x2d);
    write_data(0x29);
    write_data(0x25);
    write_data(0x2b);
    write_data(0x39);
    write_data(0x00);
    write_data(0x01);
    write_data(0x03);
    write_data(0x10);
    write_command(ST7735_GMCTRN1);
    write_data(0x03);
    write_data(0x1d);
    write_data(0x07);
    write_data(0x06);
    write_data(0x2e);
    write_data(0x2c);
    write_data(0x29);
    write_data(0x2d);
    write_data(0x2e);
    write_data(0x2e);
    write_data(0x37);
    write_data(0x3f);
    write_data(0x00);
    write_data(0x00);
    write_data(0x02);
    write_data(0x10);

    write_command(ST7735_NORON);
    nrf_delay_ms(10);
    write_command(ST7735_DISPON);
    nrf_delay_ms(100);

    if (m_st7735.tab_color == INITR_BLACKTAB)
    {
        write_command(ST7735_MADCTL);
        write_data(0xC0);
    }
}


static ret_code_t hardware_init(void)
{
    ret_code_t err_code;

    nrf_gpio_cfg_output(ST7735_DC_PIN);

    nrfx_spi_config_t spi_config = NRFX_SPI_DEFAULT_CONFIG;

    spi_config.sck_pin  = ST7735_SCK_PIN;
    spi_config.miso_pin = ST7735_MISO_PIN;
    spi_config.mosi_pin = ST7735_MOSI_PIN;
    spi_config.ss_pin   = ST7735_SS_PIN;
    spi_config.frequency = SPI_FREQUENCY_FREQUENCY_M8;

    err_code = nrfx_spi_init(&spi, &spi_config, NULL, NULL);
    return err_code;
}

static ret_code_t st7735_init(void)
{
    ret_code_t err_code;

    m_st7735.tab_color = ST7735_TAB_COLOR;

    err_code = hardware_init();
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    command_list();

    return err_code;
}

static void st7735_uninit(void)
{
    nrfx_spi_uninit(&spi);
}

static void st7735_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
    set_addr_window(x, y, x, y);

    color = RGB2BGR(color);

    const uint8_t data[2] = {color >> 8, color};

    nrf_gpio_pin_set(ST7735_DC_PIN);

    spi_write(data, sizeof(data));

    nrf_gpio_pin_clear(ST7735_DC_PIN);
}

static void st7735_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    set_addr_window(x, y, x + width - 1, y + height - 1);

    color = RGB2BGR(color);

    const uint8_t data[2] = {color >> 8, color};

    nrf_gpio_pin_set(ST7735_DC_PIN);

    // Duff's device algorithm for optimizing loop.
    uint32_t i = (height * width + 7) / 8;

/*lint -save -e525 -e616 -e646 */
    switch ((height * width) % 8) {
        case 0:
            do {
                spi_write(data, sizeof(data));
        case 7:
                spi_write(data, sizeof(data));
        case 6:
                spi_write(data, sizeof(data));
        case 5:
                spi_write(data, sizeof(data));
        case 4:
                spi_write(data, sizeof(data));
        case 3:
                spi_write(data, sizeof(data));
        case 2:
                spi_write(data, sizeof(data));
        case 1:
                spi_write(data, sizeof(data));
            } while (--i > 0);
        default:
            break;
    }
/*lint -restore */
    nrf_gpio_pin_clear(ST7735_DC_PIN);
}

static void st7735_dummy_display(uint8_t * data, uint16_t len, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    if(len == 0xFFFF){
        set_addr_window(x0, y0, x1, y1);
    }
    else{
      //spi_write(&data, len);
      write_data_buffered(data, len);
    }
}

static void st7735_rotation_set(nrf_lcd_rotation_t rotation)
{
    write_command(ST7735_MADCTL);
    switch (rotation) {
        case NRF_LCD_ROTATE_0:
            if (m_st7735.tab_color == INITR_BLACKTAB)
            {
                write_data(ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_RGB);
            }
            else
            {
                write_data(ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_BGR);
            }
            break;
        case NRF_LCD_ROTATE_90:
            if (m_st7735.tab_color == INITR_BLACKTAB)
            {
                write_data(ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_RGB);
            }
            else
            {
                write_data(ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_BGR);
            }
            break;
        case NRF_LCD_ROTATE_180:
            if (m_st7735.tab_color == INITR_BLACKTAB)
            {
                write_data(ST7735_MADCTL_RGB);
            }
            else
            {
                write_data(ST7735_MADCTL_BGR);
            }
            break;
        case NRF_LCD_ROTATE_270:
            if (m_st7735.tab_color == INITR_BLACKTAB)
            {
                write_data(ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_RGB);
            }
            else
            {
                write_data(ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_BGR);
            }
            break;
        default:
            break;
    }
}


static void st7735_display_invert(bool invert)
{
    write_command(invert ? ST7735_INVON : ST7735_INVOFF);
}

static lcd_cb_t st7735_cb = {
    .height = ST7735_HEIGHT,
    .width = ST7735_WIDTH
};

const nrf_lcd_t nrf_lcd_st7735 = {
    .lcd_init = st7735_init,
    .lcd_uninit = st7735_uninit,
    .lcd_pixel_draw = st7735_pixel_draw,
    .lcd_rect_draw = st7735_rect_draw,
    .lcd_display = st7735_dummy_display,
    .lcd_rotation_set = st7735_rotation_set,
    .lcd_display_invert = st7735_display_invert,
    .p_lcd_cb = &st7735_cb
};



#endif // NRF_MODULE_ENABLED(ST7735)