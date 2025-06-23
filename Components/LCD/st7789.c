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

#ifndef ST7789_C__
#define ST7789_C__

#include "nrf_lcd.h"
#include "nrfx_spim.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"

// Set of commands described in ST7789 data sheet.
#define ST7789_NOP     0x00  // No operation
#define ST7789_SWRESET 0x01  // Software reset
#define ST7789_RDDID   0x04  // Read display ID
#define ST7789_RDDST   0x09  // Read display status

#define ST7789_SLPIN   0x10  // Enter sleep mode
#define ST7789_SLPOUT  0x11  // Exit sleep mode
#define ST7789_PTLON   0x12  // Partial mode ON
#define ST7789_NORON   0x13  // Normal display mode ON

#define ST7789_INVOFF  0x20  // Display inversion OFF
#define ST7789_INVON   0x21  // Display inversion ON
#define ST7789_DISPOFF 0x28  // Display OFF
#define ST7789_DISPON  0x29  // Display ON
#define ST7789_CASET   0x2A  // Column address set
#define ST7789_RASET   0x2B  // Row address set
#define ST7789_RAMWR   0x2C  // Write to RAM
#define ST7789_RAMRD   0x2E  // Read from RAM

#define ST7789_PTLAR   0x30  // Partial area set
#define ST7789_COLMOD  0x3A  // Interface pixel format
#define ST7789_MADCTL  0x36  // Memory data access control

#define ST7789_FRMCTR1 0xB1  // Frame rate control (normal mode)
#define ST7789_FRMCTR2 0xB2  // Frame rate control (idle mode)
#define ST7789_FRMCTR3 0xB3  // Frame rate control (partial mode)
#define ST7789_INVCTR  0xB4  // Display inversion control
#define ST7789_DISSET5 0xB6  // Display function control

#define ST7789_PWCTR1  0xC0  // Power control 1
#define ST7789_PWCTR2  0xC1  // Power control 2
#define ST7789_PWCTR3  0xC2  // Power control 3
#define ST7789_PWCTR4  0xC3  // Power control 4
#define ST7789_PWCTR5  0xC4  // Power control 5
#define ST7789_VMCTR1  0xC5  // VCOM control 1

#define ST7789_RDID1   0xDA  // Read ID 1
#define ST7789_RDID2   0xDB  // Read ID 2
#define ST7789_RDID3   0xDC  // Read ID 3
#define ST7789_RDID4   0xDD  // Read ID 4

#define ST7789_PWCTR6  0xFC  // Power control 6

#define ST7789_GMCTRP1 0xE0  // Positive gamma correction
#define ST7789_GMCTRN1 0xE1  // Negative gamma correction

// MADCTL parameters (Memory Access Control)
#define ST7789_MADCTL_MY  0x80  // Row address order (Y mirror)
#define ST7789_MADCTL_MX  0x40  // Column address order (X mirror)
#define ST7789_MADCTL_MV  0x20  // Row/column exchange (X-Y swap)
#define ST7789_MADCTL_ML  0x10  // Vertical refresh order
#define ST7789_MADCTL_RGB 0x00  // RGB color order
#define ST7789_MADCTL_BGR 0x08  // BGR color order
#define ST7789_MADCTL_MH  0x04  // Horizontal refresh order


#define ST7789_COLOR_MODE_12BIT  0x03
#define ST7789_COLOR_MODE_16BIT  0x05  // RGB565
#define ST7789_COLOR_MODE_18BIT  0x06  // RGB666

#define RGB2BGR(x)      (x << 11) | (x & 0x07E0) | (x >> 11)

typedef enum {
    UNINITIALISED,
    IDLE,
    SEND_SWRESET_CMD,
    SEND_SLPOUT_CMD,
    SEND_COLMOD_CMD,
    SEND_COLMOD_DATA,
    SEND_MADCTL_CMD,
    SEND_MADCTL_DATA,
    SEND_INVON_CMD,
    SEND_NORON_CMD,
    SEND_DISPON_CMD,
    SEND_CASET_CMD,
    SEND_CASET_DATA,
    SEND_RASET_CMD,
    SEND_RASET_DATA,
    SEND_DATA_BUFFERED,
    SEND_RAMWR_CMD,
    SEND_PIXEL_DATA,

} st7789_state_t;

st7789_state_t currentState = UNINITIALISED;
st7789_state_t nextState;

uint8_t xPositions[4];
uint8_t yPositions[4];

uint8_t * pixelData;
uint16_t pixelDataLength;

static inline nrfx_err_t st7789_spi_write(const nrfx_spim_t * spim, const void * data, size_t size)
{
    nrfx_spim_xfer_desc_t desc;

    desc.p_tx_buffer = data;
    desc.tx_length = size;
    desc.p_rx_buffer = NULL;
    desc.rx_length = 0;

    return nrfx_spim_xfer(spim, &desc, 0);
}

static inline nrfx_err_t st7789_write_data_buffered(const nrfx_spim_t * spim, uint8_t dc_pin, uint8_t * c, uint16_t len)
{
    nrf_gpio_pin_set(dc_pin);

    return st7789_spi_write(spim, c, len);
}

static inline nrfx_err_t st7789_write_command(const nrfx_spim_t * spim, uint8_t dc_pin, uint8_t c)
{
    nrf_gpio_pin_clear(dc_pin);
    return st7789_spi_write(spim, &c, sizeof(c));
}

static inline nrfx_err_t st7789_write_data(const nrfx_spim_t * spim, uint8_t dc_pin, uint8_t c)
{
    nrf_gpio_pin_set(dc_pin);
    return st7789_spi_write(spim, &c, sizeof(c));
}

static void st7789_set_addr_window(const nrfx_spim_t * spim, uint8_t dc_pin, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    ASSERT(x0 <= x1);
    ASSERT(y0 <= y1);

    x0 += 0;  // Add horizontal offset
    y0 += 34; // Add vertical offset
    x1 += 0;
    y1 += 34;

    xPositions[0] = x0 >> 8;
    xPositions[1] = x0 & 0xFF;
    xPositions[2] = x1 >> 8;
    xPositions[3] = x1 & 0xFF;

    yPositions[0] = y0 >> 8;
    yPositions[1] = y0 & 0xFF;
    yPositions[2] = y1 >> 8;
    yPositions[3] = y1 & 0xFF;
}

static void st7789_send_setup_command_list(const nrfx_spim_t * spim, uint8_t dc_pin)
{
    // Software Reset
    st7789_write_command(spim, dc_pin, ST7789_SWRESET);
    nrf_delay_ms(150);
    // Exit Sleep Mode
    st7789_write_command(spim, dc_pin, ST7789_SLPOUT);
    nrf_delay_ms(500);

    // Set the pixel format (color depth) to 16 bits per pixel (65K colors)
    st7789_write_command(spim, dc_pin, ST7789_COLMOD);
    st7789_write_data(spim, dc_pin, ST7789_COLOR_MODE_16BIT);

    // Set the Memory Data Access Control (MADCTL) register to 0x08
    // This sets color order to BGR and default orientation
    st7789_write_command(spim, dc_pin, ST7789_MADCTL);
    st7789_write_data(spim, dc_pin, ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);

    // Turn on display inversion (makes blacks white and whites black — used for contrast adjustment)
    // Turn on display inversion (makes blacks white and whites black — used for contrast adjustment)
    st7789_write_command(spim, dc_pin, ST7789_INVON);

    // Exit sleep mode, normal display mode on
    st7789_write_command(spim, dc_pin, ST7789_NORON);

    // Turn the display on (enable screen output)
    nrf_delay_ms(10);
    st7789_write_command(spim, dc_pin, ST7789_DISPON);
    nrf_delay_ms(100);
}

static void st7789_xfer_complete_handler(const nrfx_spim_t * spim, uint8_t dc_pin)
{
    ret_code_t err_code;
    switch (nextState)
    {
        case SEND_SWRESET_CMD:
            // Software Reset
            nextState = SEND_SLPOUT_CMD;
            st7789_write_command(spim, dc_pin, ST7789_SWRESET);
            break;
        case SEND_SLPOUT_CMD:
            nextState = SEND_COLMOD_CMD;
            // Exit Sleep Mode
            st7789_write_command(spim, dc_pin, ST7789_SLPOUT);
            break;
        case SEND_COLMOD_CMD:
            nextState = SEND_COLMOD_DATA;
            // Set the pixel format (color depth) to 16 bits per pixel (65K colors)
            st7789_write_command(spim, dc_pin, ST7789_COLMOD);

            break;
        case SEND_COLMOD_DATA:
            nextState = SEND_MADCTL_CMD;
            st7789_write_data(spim, dc_pin, ST7789_COLOR_MODE_16BIT);

            break;
        case SEND_MADCTL_CMD:
            nextState = SEND_MADCTL_DATA;
            // Set the Memory Data Access Control (MADCTL) register to 0x08
            // This sets color order to BGR and default orientation
            st7789_write_command(spim, dc_pin, ST7789_MADCTL);

            break;
        case SEND_MADCTL_DATA:
            nextState = SEND_INVON_CMD;
            st7789_write_data(spim, dc_pin, ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);

            break;
        case SEND_INVON_CMD:
            nextState = SEND_NORON_CMD;
            // Turn on display inversion (makes blacks white and whites black — used for contrast adjustment)
            st7789_write_command(spim, dc_pin, ST7789_INVON);

            break;
        case SEND_NORON_CMD:
            nextState = SEND_DISPON_CMD;
            // Exit sleep mode, normal display mode on
            st7789_write_command(spim, dc_pin, ST7789_NORON);

            break;
        case SEND_DISPON_CMD:
            nextState = IDLE;
            st7789_write_command(spim, dc_pin, ST7789_DISPON);
            break;
        case SEND_CASET_DATA:
            nextState = SEND_RASET_CMD;
            st7789_write_data_buffered(spim, dc_pin, xPositions, sizeof(xPositions));
            break;
        
        case SEND_RASET_CMD:
            nextState = SEND_RASET_DATA;
            // Turn the display on (enable screen output)
            st7789_write_command(spim, dc_pin, ST7789_RASET);
            break;
        case SEND_RASET_DATA:
            nextState = SEND_RAMWR_CMD;
            // Turn the display on (enable screen output)
            st7789_write_data_buffered(spim, dc_pin, yPositions, sizeof(yPositions));
            break;
        
        case SEND_RAMWR_CMD:
            nextState = SEND_PIXEL_DATA;
            // Turn the display on (enable screen output)
            st7789_write_command(spim, dc_pin, ST7789_RAMWR);
            break;
        
        case SEND_PIXEL_DATA:
            
            nextState = IDLE;
            // Turn the display on (enable screen output)
            err_code = st7789_write_data_buffered(spim, dc_pin, pixelData, pixelDataLength);
            break;
        
        default:
            nextState = IDLE;
            break;
    }
}

static ret_code_t st7789_init(const nrfx_spim_t * spim, uint8_t dc_pin)
{
    nextState = SEND_SWRESET_CMD;
    //st7789_send_setup_command_list(spim, dc_pin);
    st7789_xfer_complete_handler(spim, dc_pin);

    return NRF_SUCCESS;
}

static void st7789_uninit()
{
    nextState = UNINITIALISED;
}

static nrfx_err_t st7789_display(const nrfx_spim_t * spim, uint8_t dc_pin, uint8_t * data, uint16_t len, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    ret_code_t err_code;
    pixelData = data;
    pixelDataLength = len;

    if (nextState == IDLE)
    {
        st7789_set_addr_window(spim, dc_pin, x0, y0, x1, y1);

        //NRF_LOG_INFO("SEND_ST7789_CASET");
        nextState = SEND_CASET_DATA;
        err_code = st7789_write_command(spim, dc_pin, ST7789_CASET);
    }
    else
    {
        err_code = NRF_ERROR_FORBIDDEN;
    }

    return err_code;
}

static void st7789_rotation_set(const nrfx_spim_t * spim, uint8_t dc_pin, nrf_lcd_rotation_t rotation)
{
    st7789_write_command(spim, dc_pin, ST7789_MADCTL);

    switch (rotation) {
        case NRF_LCD_ROTATE_0:
            // Default portrait: mirror X and Y, RGB order
            st7789_write_data(spim, dc_pin,
                ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
            break;

        case NRF_LCD_ROTATE_90:
            // Landscape: mirror Y and swap rows/cols (MV), RGB order
            st7789_write_data(spim, dc_pin,
                ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
            break;

        case NRF_LCD_ROTATE_180:
            // Portrait upside-down: no mirror, RGB order
            st7789_write_data(spim, dc_pin,
                ST7789_MADCTL_RGB);
            break;

        case NRF_LCD_ROTATE_270:
            // Landscape inverted: mirror X, swap rows/cols (MV), RGB order
            st7789_write_data(spim, dc_pin,
                ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
            break;

        default:
            // Optionally handle invalid rotation
            // e.g., apply default or log error
            st7789_write_data(spim, dc_pin,
                ST7789_MADCTL_RGB);
            break;
    }
}


static void st7789_display_invert(const nrfx_spim_t * spim, uint8_t dc_pin, bool invert)
{
    st7789_write_command(spim, dc_pin, invert ? ST7789_INVON : ST7789_INVOFF);
}


const nrf_lcd_t nrf_lcd_st7789 = {
    .lcd_init = st7789_init,
    .lcd_uninit = st7789_uninit,
    .lcd_display = st7789_display,
    .lcd_rotation_set = st7789_rotation_set,
    .lcd_display_invert = st7789_display_invert,
    .xfer_complete_handler = st7789_xfer_complete_handler,
};



#endif // NRF_MODULE_ENABLED(ST7789)
