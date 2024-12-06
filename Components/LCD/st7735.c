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

#include "nrf_lcd.h"
#include "nrfx_spim.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"

// Set of commands described in ST7735 data sheet.
// Commands without parameters
#define ST7735_NOP     0x00       // No Operation

// Software reset command
#define ST7735_SWRESET 0x01       // Software Reset

// Read Display ID command
#define ST7735_RDDID   0x04       // Read Display ID

// Read Display Status command
#define ST7735_RDDST   0x09       // Read Display Status

// Sleep In and Sleep Out commands
#define ST7735_SLPIN   0x10       // Sleep In
#define ST7735_SLPOUT  0x11       // Sleep Out

// Partial mode and Normal mode commands
#define ST7735_PTLON   0x12       // Partial mode On
#define ST7735_NORON   0x13       // Normal mode On

// Display inversion commands
#define ST7735_INVOFF  0x20       // Display Inversion Off
#define ST7735_INVON   0x21       // Display Inversion On

// Display On/Off commands
#define ST7735_DISPOFF 0x28       // Display Off
#define ST7735_DISPON  0x29       // Display On

// Column and Row Address Set commands
#define ST7735_CASET   0x2A       // Column Address Set
#define ST7735_RASET   0x2B       // Row Address Set

// Memory Write and Read commands
#define ST7735_RAMWR   0x2C       // Memory Write
#define ST7735_RAMRD   0x2E       // Memory Read

// Partial Area and Partial Mode commands
#define ST7735_PTLAR   0x30       // Partial Area
#define ST7735_COLMOD  0x3A       // Interface Pixel Format
#define ST7735_MADCTL  0x36       // Memory Data Access Control

// Frame Rate Control and Inversion Control commands
#define ST7735_FRMCTR1 0xB1       // Frame Rate Control (In Normal Mode/Full Colors)
#define ST7735_FRMCTR2 0xB2       // Frame Rate Control (In Idle Mode/8 colors)
#define ST7735_FRMCTR3 0xB3       // Frame Rate Control (In Partial Mode/Full Colors)
#define ST7735_INVCTR  0xB4       // Display Inversion Control
#define ST7735_DISSET5 0xB6       // Display Function Set 5

// Power Control commands
#define ST7735_PWCTR1  0xC0       // Power Control 1
#define ST7735_PWCTR2  0xC1       // Power Control 2
#define ST7735_PWCTR3  0xC2       // Power Control 3
#define ST7735_PWCTR4  0xC3       // Power Control 4
#define ST7735_PWCTR5  0xC4       // Power Control 5
#define ST7735_VMCTR1  0xC5       // VCOM Control 1

// Read ID commands
#define ST7735_RDID1   0xDA       // Read ID 1
#define ST7735_RDID2   0xDB       // Read ID 2
#define ST7735_RDID3   0xDC       // Read ID 3
#define ST7735_RDID4   0xDD       // Read ID 4

// Power Control 6 command
#define ST7735_PWCTR6  0xFC       // Power Control 6

// Positive and Negative Gamma Correction commands
#define ST7735_GMCTRP1 0xE0       // Positive Gamma Correction
#define ST7735_GMCTRN1 0xE1       // Negative Gamma Correction

// Memory Data Access Control bits
#define ST7735_MADCTL_MY  0x80    // Row Address Order
#define ST7735_MADCTL_MX  0x40    // Column Address Order
#define ST7735_MADCTL_MV  0x20    // Row/Column Exchange
#define ST7735_MADCTL_ML  0x10    // Vertical Refresh Order
#define ST7735_MADCTL_RGB 0x00    // Red-Green-Blue Pixel Order
#define ST7735_MADCTL_BGR 0x08    // Blue-Green-Red Pixel Order
#define ST7735_MADCTL_MH  0x04    // Horizontal Refresh Order

/* @} */

#define RGB2BGR(x)      (x << 11) | (x & 0x07E0) | (x >> 11)


#ifndef ST7735_SPI_INSTANCE
#define ST7735_SPI_INSTANCE 3
#endif

static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(ST7735_SPI_INSTANCE);  /**< SPI instance. */

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


static inline void st7735_spi_write(const nrfx_spim_t * spim, uint8_t dc_pin, const void * data, size_t size)
{
    nrfx_spim_xfer_desc_t desc;

    desc.p_tx_buffer = data;
    desc.tx_length = size;
    desc.p_rx_buffer = NULL;
    desc.rx_length = 0;

    (nrfx_spim_xfer(&spi, &desc, 0));
}

static inline void st7735_write_data_buffered(const nrfx_spim_t * spim, uint8_t dc_pin, uint8_t * c, uint16_t len)
{
    nrf_gpio_pin_set(dc_pin);

    st7735_spi_write(spim, dc_pin, c, len);
}

static inline void st7735_write_command(const nrfx_spim_t * spim, uint8_t dc_pin, uint8_t c)
{
    nrf_gpio_pin_clear(dc_pin);
    st7735_spi_write(spim, dc_pin, &c, sizeof(c));
}

static inline void st7735_write_data(const nrfx_spim_t * spim, uint8_t dc_pin, uint8_t c)
{
    nrf_gpio_pin_set(dc_pin);
    st7735_spi_write(spim, dc_pin, &c, sizeof(c));
}

static void st7735_set_addr_window(const nrfx_spim_t * spim, uint8_t dc_pin, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    ASSERT(x0 <= x1);
    ASSERT(y0 <= y1);

    x0 += 1;  // Add horizontal offset
    y0 += 26; // Add vertical offset
    x1 += 1;
    y1 += 26;
    
    st7735_write_command(spim, dc_pin, ST7735_CASET);
    st7735_write_data(spim, dc_pin, 0x00);                       // For a 128x160 display, it is always 0.
    st7735_write_data(spim, dc_pin, x0);
    st7735_write_data(spim, dc_pin, 0x00);                       // For a 128x160 display, it is always 0.
    st7735_write_data(spim, dc_pin, x1);
    st7735_write_command(spim, dc_pin, ST7735_RASET);
    st7735_write_data(spim, dc_pin, 0x00);                       // For a 128x160 display, it is always 0.
    st7735_write_data(spim, dc_pin, y0);
    st7735_write_data(spim, dc_pin, 0x00);                       // For a 128x160 display, it is always 0.
    st7735_write_data(spim, dc_pin, y1);
    st7735_write_command(spim, dc_pin, ST7735_RAMWR);
}

static void st7735_command_list(const nrfx_spim_t * spim, uint8_t dc_pin)
{
    st7735_write_command(spim, dc_pin, ST7735_SWRESET);
    nrf_delay_ms(150);
    st7735_write_command(spim, dc_pin, ST7735_SLPOUT);
    nrf_delay_ms(500);

    st7735_write_command(spim, dc_pin, ST7735_FRMCTR1);
    st7735_write_data(spim, dc_pin, 0x01);
    st7735_write_data(spim, dc_pin, 0x2C);
    st7735_write_data(spim, dc_pin, 0x2D);
    st7735_write_command(spim, dc_pin, ST7735_FRMCTR2);
    st7735_write_data(spim, dc_pin, 0x01);
    st7735_write_data(spim, dc_pin, 0x2C);
    st7735_write_data(spim, dc_pin, 0x2D);
    st7735_write_command(spim, dc_pin, ST7735_FRMCTR3);
    st7735_write_data(spim, dc_pin, 0x01);
    st7735_write_data(spim, dc_pin, 0x2C);
    st7735_write_data(spim, dc_pin, 0x2D);
    st7735_write_data(spim, dc_pin, 0x01);
    st7735_write_data(spim, dc_pin, 0x2C);
    st7735_write_data(spim, dc_pin, 0x2D);

    st7735_write_command(spim, dc_pin, ST7735_INVCTR);
    st7735_write_data(spim, dc_pin, 0x07);

    st7735_write_command(spim, dc_pin, ST7735_PWCTR1);
    st7735_write_data(spim, dc_pin, 0xA2);
    st7735_write_data(spim, dc_pin, 0x02);
    st7735_write_data(spim, dc_pin, 0x84);
    st7735_write_command(spim, dc_pin, ST7735_PWCTR2);
    st7735_write_data(spim, dc_pin, 0xC5);
    st7735_write_command(spim, dc_pin, ST7735_PWCTR3);
    st7735_write_data(spim, dc_pin, 0x0A);
    st7735_write_data(spim, dc_pin, 0x00);
    st7735_write_command(spim, dc_pin, ST7735_PWCTR3);
    st7735_write_data(spim, dc_pin, 0x8A);
    st7735_write_data(spim, dc_pin, 0x2A);
    st7735_write_command(spim, dc_pin, ST7735_PWCTR5);
    st7735_write_data(spim, dc_pin, 0x8A);
    st7735_write_data(spim, dc_pin, 0xEE);
    st7735_write_data(spim, dc_pin, 0x0E);

    st7735_write_command(spim, dc_pin, ST7735_INVOFF);
    st7735_write_command(spim, dc_pin, ST7735_MADCTL);
    st7735_write_data(spim, dc_pin, 0xC8);

    st7735_write_command(spim, dc_pin, ST7735_COLMOD);
    st7735_write_data(spim, dc_pin, 0x05);

    if (m_st7735.tab_color == INITR_GREENTAB)
    {
        st7735_write_command(spim, dc_pin, ST7735_CASET);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x02);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x81);
        st7735_write_command(spim, dc_pin, ST7735_RASET);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x01);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0xA0);
    }
    else if (m_st7735.tab_color == INITR_144GREENTAB)
    {
        st7735_write_command(spim, dc_pin, ST7735_CASET);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x7F);
        st7735_write_command(spim, dc_pin, ST7735_RASET);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x7F);
    }
    else if (m_st7735.tab_color == INITR_REDTAB)
    {
        st7735_write_command(spim, dc_pin, ST7735_CASET);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x7F);
        st7735_write_command(spim, dc_pin, ST7735_RASET);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x9F);
    }
    else if (m_st7735.tab_color == INITR_BLUETAB)
    {
        st7735_write_command(spim, dc_pin, ST7735_CASET);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x02);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_command(spim, dc_pin, ST7735_RASET);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0x1B);
        st7735_write_data(spim, dc_pin, 0x00);
        st7735_write_data(spim, dc_pin, 0xBB);
    }

    st7735_write_command(spim, dc_pin, ST7735_GMCTRP1);
    st7735_write_data(spim, dc_pin, 0x02);
    st7735_write_data(spim, dc_pin, 0x1c);
    st7735_write_data(spim, dc_pin, 0x07);
    st7735_write_data(spim, dc_pin, 0x12);
    st7735_write_data(spim, dc_pin, 0x37);
    st7735_write_data(spim, dc_pin, 0x32);
    st7735_write_data(spim, dc_pin, 0x29);
    st7735_write_data(spim, dc_pin, 0x2d);
    st7735_write_data(spim, dc_pin, 0x29);
    st7735_write_data(spim, dc_pin, 0x25);
    st7735_write_data(spim, dc_pin, 0x2b);
    st7735_write_data(spim, dc_pin, 0x39);
    st7735_write_data(spim, dc_pin, 0x00);
    st7735_write_data(spim, dc_pin, 0x01);
    st7735_write_data(spim, dc_pin, 0x03);
    st7735_write_data(spim, dc_pin, 0x10);
    st7735_write_command(spim, dc_pin, ST7735_GMCTRN1);
    st7735_write_data(spim, dc_pin, 0x03);
    st7735_write_data(spim, dc_pin, 0x1d);
    st7735_write_data(spim, dc_pin, 0x07);
    st7735_write_data(spim, dc_pin, 0x06);
    st7735_write_data(spim, dc_pin, 0x2e);
    st7735_write_data(spim, dc_pin, 0x2c);
    st7735_write_data(spim, dc_pin, 0x29);
    st7735_write_data(spim, dc_pin, 0x2d);
    st7735_write_data(spim, dc_pin, 0x2e);
    st7735_write_data(spim, dc_pin, 0x2e);
    st7735_write_data(spim, dc_pin, 0x37);
    st7735_write_data(spim, dc_pin, 0x3f);
    st7735_write_data(spim, dc_pin, 0x00);
    st7735_write_data(spim, dc_pin, 0x00);
    st7735_write_data(spim, dc_pin, 0x02);
    st7735_write_data(spim, dc_pin, 0x10);

    st7735_write_command(spim, dc_pin, ST7735_NORON);
    nrf_delay_ms(10);
    st7735_write_command(spim, dc_pin, ST7735_DISPON);
    nrf_delay_ms(100);

    if (m_st7735.tab_color == INITR_BLACKTAB)
    {
        st7735_write_command(spim, dc_pin, ST7735_MADCTL);
        st7735_write_data(spim, dc_pin, 0xC0);
    }
}

static ret_code_t st7735_init(const nrfx_spim_t * spim, uint8_t dc_pin)
{
    ret_code_t err_code;

    st7735_command_list(spim, dc_pin);

    return err_code;
}

static void st7735_uninit(void)
{
    
}

static void st7735_pixel_draw(const nrfx_spim_t * spim, uint8_t dc_pin, uint16_t x, uint16_t y, uint32_t color)
{
    st7735_set_addr_window(spim, dc_pin, x, y, x, y);

    color = RGB2BGR(color);

    const uint8_t data[2] = {color >> 8, color};

    nrf_gpio_pin_set(dc_pin);

    st7735_spi_write(spim, dc_pin, data, sizeof(data));

    nrf_gpio_pin_clear(dc_pin);
}

static void st7735_rect_draw(const nrfx_spim_t * spim, uint8_t dc_pin, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    st7735_set_addr_window(spim, dc_pin, x, y, x + width - 1, y + height - 1);

    color = RGB2BGR(color);

    const uint8_t data[2] = {color >> 8, color};

    nrf_gpio_pin_set(dc_pin);

    // Duff's device algorithm for optimizing loop.
    uint32_t i = (height * width + 7) / 8;

/*lint -save -e525 -e616 -e646 */
    switch ((height * width) % 8) {
        case 0:
            do {
                st7735_spi_write(spim, dc_pin, data, sizeof(data));
        case 7:
                st7735_spi_write(spim, dc_pin, data, sizeof(data));
        case 6:
                st7735_spi_write(spim, dc_pin, data, sizeof(data));
        case 5:
                st7735_spi_write(spim, dc_pin, data, sizeof(data));
        case 4:
                st7735_spi_write(spim, dc_pin, data, sizeof(data));
        case 3:
                st7735_spi_write(spim, dc_pin, data, sizeof(data));
        case 2:
                st7735_spi_write(spim, dc_pin, data, sizeof(data));
        case 1:
                st7735_spi_write(spim, dc_pin, data, sizeof(data));
            } while (--i > 0);
        default:
            break;
    }
/*lint -restore */
    nrf_gpio_pin_clear(dc_pin);
}

static void st7735_set_addr_window_to_buffer(const nrfx_spim_t * spim, uint8_t dc_pin, uint8_t * data, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    x0 += 1;  // Add horizontal offset
    y0 += 26; // Add vertical offset
    x1 += 1;
    y1 += 26;
    
    data[0] = ST7735_CASET;
    data[1] = 0x00;
    data[2] = x0;
    data[3] = 0x00;
    data[4] = x1;
    data[5] = ST7735_RASET;
    data[6] = 0x00;
    data[7] = y0;
    data[8] = 0x00;
    data[9] = y1;
    data[10] = ST7735_RAMWR;

    st7735_write_command(spim, dc_pin, ST7735_CASET);
    st7735_write_data(spim, dc_pin, 0x00);                       // For a 128x160 display, it is always 0.
    st7735_write_data(spim, dc_pin, x0);
    st7735_write_data(spim, dc_pin, 0x00);                       // For a 128x160 display, it is always 0.
    st7735_write_data(spim, dc_pin, x1);
    st7735_write_command(spim, dc_pin, ST7735_RASET);
    st7735_write_data(spim, dc_pin, 0x00);                       // For a 128x160 display, it is always 0.
    st7735_write_data(spim, dc_pin, y0);
    st7735_write_data(spim, dc_pin, 0x00);                       // For a 128x160 display, it is always 0.
    st7735_write_data(spim, dc_pin, y1);
    st7735_write_command(spim, dc_pin, ST7735_RAMWR);
}

static void st7735_display(const nrfx_spim_t * spim, uint8_t dc_pin, uint8_t * data, uint16_t len, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    st7735_set_addr_window(spim, dc_pin, x0, y0, x1, y1);

    st7735_write_data_buffered(spim, dc_pin, data, len);   
}

static void st7735_rotation_set(const nrfx_spim_t * spim, uint8_t dc_pin, nrf_lcd_rotation_t rotation)
{
    st7735_write_command(spim, dc_pin, ST7735_MADCTL);
    switch (rotation) {
        case NRF_LCD_ROTATE_0:
            if (m_st7735.tab_color == INITR_BLACKTAB)
            {
                st7735_write_data(spim, dc_pin, ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_RGB);
            }
            else
            {
                st7735_write_data(spim, dc_pin, ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_BGR);
            }
            break;
        case NRF_LCD_ROTATE_90:
            if (m_st7735.tab_color == INITR_BLACKTAB)
            {
                st7735_write_data(spim, dc_pin, ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_RGB);
            }
            else
            {
                st7735_write_data(spim, dc_pin, ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_BGR);
            }
            break;
        case NRF_LCD_ROTATE_180:
            if (m_st7735.tab_color == INITR_BLACKTAB)
            {
                st7735_write_data(spim, dc_pin, ST7735_MADCTL_RGB);
            }
            else
            {
                st7735_write_data(spim, dc_pin, ST7735_MADCTL_BGR);
            }
            break;
        case NRF_LCD_ROTATE_270:
            if (m_st7735.tab_color == INITR_BLACKTAB)
            {
                st7735_write_data(spim, dc_pin, ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_RGB);
            }
            else
            {
                st7735_write_data(spim, dc_pin, ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_BGR);
            }
            break;
        default:
            break;
    }
}


static void st7735_display_invert(const nrfx_spim_t * spim, uint8_t dc_pin, bool invert)
{
    st7735_write_command(spim, dc_pin, invert ? ST7735_INVON : ST7735_INVOFF);
}


const nrf_lcd_t nrf_lcd_st7735 = {
    .lcd_init = st7735_init,
    .lcd_uninit = st7735_uninit,
    .lcd_pixel_draw = st7735_pixel_draw,
    .lcd_rect_draw = st7735_rect_draw,
    .lcd_display = st7735_display,
    .lcd_rotation_set = st7735_rotation_set,
    .lcd_display_invert = st7735_display_invert,
    .lcd_set_addr_window_to_buffer = st7735_set_addr_window_to_buffer,
};

