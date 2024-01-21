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

#ifndef ST7789_DC_PIN
#define ST7789_DC_PIN 16
#endif

#ifndef ST7789_HEIGHT
#define ST7789_HEIGHT 172
#endif


#ifndef ST7789_WIDTH
#define ST7789_WIDTH 320
#endif


#ifndef ST7789_SCK_PIN
#define ST7789_SCK_PIN 14
#endif

// <o> ST7789_MISO_PIN - Pin number  <0-47> 


#ifndef ST7789_MISO_PIN
#define ST7789_MISO_PIN 12
#endif

// <o> ST7789_MOSI_PIN - Pin number  <0-47> 


#ifndef ST7789_MOSI_PIN
#define ST7789_MOSI_PIN 12
#endif

// <o> ST7789_SS_PIN - Pin number  <0-47> 


#ifndef ST7789_SS_PIN
#define ST7789_SS_PIN 17
#endif

#ifndef ST7789_C__
#define ST7789_C__

#include "nrf_lcd.h"
#include "nrfx_spim.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"

// Set of commands described in ST7789 data sheet.
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID   0x04
#define ST7789_RDDST   0x09

#define ST7789_SLPIN   0x10
#define ST7789_SLPOUT  0x11
#define ST7789_PTLON   0x12
#define ST7789_NORON   0x13

#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E

#define ST7789_PTLAR   0x30
#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36

#define ST7789_FRMCTR1 0xB1
#define ST7789_FRMCTR2 0xB2
#define ST7789_FRMCTR3 0xB3
#define ST7789_INVCTR  0xB4
#define ST7789_DISSET5 0xB6

#define ST7789_PWCTR1  0xC0
#define ST7789_PWCTR2  0xC1
#define ST7789_PWCTR3  0xC2
#define ST7789_PWCTR4  0xC3
#define ST7789_PWCTR5  0xC4
#define ST7789_VMCTR1  0xC5

#define ST7789_RDID1   0xDA
#define ST7789_RDID2   0xDB
#define ST7789_RDID3   0xDC
#define ST7789_RDID4   0xDD

#define ST7789_PWCTR6  0xFC

#define ST7789_GMCTRP1 0xE0
#define ST7789_GMCTRN1 0xE1

#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_ML  0x10
#define ST7789_MADCTL_RGB 0x00
#define ST7789_MADCTL_BGR 0x08
#define ST7789_MADCTL_MH  0x04
/* @} */

#define RGB2BGR(x)      (x << 11) | (x & 0x07E0) | (x >> 11)

#define ST7789_SPI_INSTANCE 3

static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(ST7789_SPI_INSTANCE);  /**< SPI instance. */

/**
 * @brief Structure holding ST7789 controller basic parameters.
 */
typedef struct
{
    uint8_t tab_color;      /**< Color of tab attached to the used screen. */
}st7789_t;


static st7789_t m_st7789;


static inline void spi_write(const void * data, size_t size)
{
    nrfx_spim_xfer_desc_t desc;

    desc.p_tx_buffer = data;
    desc.tx_length = size;
    desc.p_rx_buffer = NULL;
    desc.rx_length = 0;

    APP_ERROR_CHECK(nrfx_spim_xfer(&spi, &desc, 0));
}

static inline void write_data_buffered(uint8_t * c, uint16_t len)
{
    nrf_gpio_pin_set(ST7789_DC_PIN);

    spi_write(c, len);
}

static inline void write_command(uint8_t c)
{
    nrf_gpio_pin_clear(ST7789_DC_PIN);
    spi_write(&c, sizeof(c));
}

static inline void write_data(uint8_t c)
{
    nrf_gpio_pin_set(ST7789_DC_PIN);
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
    
    write_command(ST7789_CASET);
    write_data(0x00);                       // For a 128x160 display, it is always 0.
    write_data(x0);
    write_data(0x00);                       // For a 128x160 display, it is always 0.
    write_data(x1);
    write_command(ST7789_RASET);
    write_data(0x00);                       // For a 128x160 display, it is always 0.
    write_data(y0);
    write_data(0x00);                       // For a 128x160 display, it is always 0.
    write_data(y1);
    write_command(ST7789_RAMWR);
}

static void command_list(void)
{
    write_command(ST7789_SWRESET);
    nrf_delay_ms(150);
    write_command(ST7789_SLPOUT);
    nrf_delay_ms(500);

    write_command(ST7789_COLMOD);
    write_data(0x55);               //     16-bit color
    write_command(ST7789_MADCTL);
    write_data(0x08);

    write_command(ST7789_CASET);
    write_data(0x00);                       // For a 128x160 display, it is always 0.
    write_data(0x00);
    write_data(0x00);                       // For a 128x160 display, it is always 0.
    write_data(240);


    write_command(ST7789_RASET);
    write_data(0x00);                       // For a 128x160 display, it is always 0.
    write_data(0);
    write_data(320>>8);                       // For a 128x160 display, it is always 0.
    write_data(320 & 0xFF);

    write_command(ST7789_INVON);

    write_command(ST7789_NORON);
    nrf_delay_ms(10);
    write_command(ST7789_DISPON);
    nrf_delay_ms(100);
}


static ret_code_t hardware_init(void)
{
    ret_code_t err_code;

    nrf_gpio_cfg_output(ST7789_DC_PIN);

    nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;

    spi_config.sck_pin  = ST7789_SCK_PIN;
    spi_config.miso_pin = ST7789_MISO_PIN;
    spi_config.mosi_pin = ST7789_MOSI_PIN;
    spi_config.ss_pin   = ST7789_SS_PIN;
    spi_config.frequency = SPI_FREQUENCY_FREQUENCY_M8;

    err_code = nrfx_spim_init(&spi, &spi_config, NULL, NULL);
    return err_code;
}

static ret_code_t st7789_init(void)
{
    ret_code_t err_code;

    err_code = hardware_init();
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    command_list();

    return err_code;
}

static void st7789_uninit(void)
{
    nrfx_spim_uninit(&spi);
}

static void st7789_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
    set_addr_window(x, y, x, y);

    color = RGB2BGR(color);

    const uint8_t data[2] = {color >> 8, color};

    nrf_gpio_pin_set(ST7789_DC_PIN);

    spi_write(data, sizeof(data));

    nrf_gpio_pin_clear(ST7789_DC_PIN);
}

static void st7789_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    set_addr_window(x, y, x + width - 1, y + height - 1);

    color = RGB2BGR(color);

    const uint8_t data[2] = {color >> 8, color};

    nrf_gpio_pin_set(ST7789_DC_PIN);

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
    nrf_gpio_pin_clear(ST7789_DC_PIN);
}

static void st7789_dummy_display(uint8_t * data, uint16_t len, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    if(len == 0xFFFF){
        set_addr_window(x0, y0, x1, y1);
    }
    else{
      //spi_write(&data, len);
      write_data_buffered(data, len);
    }
}

static void st7789_rotation_set(nrf_lcd_rotation_t rotation)
{
    write_command(ST7789_MADCTL);
    switch (rotation) {
        case NRF_LCD_ROTATE_0:

            write_data(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
            
            break;
        case NRF_LCD_ROTATE_90:

            write_data(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);

            break;
        case NRF_LCD_ROTATE_180:

            write_data(ST7789_MADCTL_RGB);

            break;
        case NRF_LCD_ROTATE_270:

            write_data(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);

            break;
        default:
            break;
    }
}


static void st7789_display_invert(bool invert)
{
    write_command(invert ? ST7789_INVON : ST7789_INVOFF);
}

static lcd_cb_t st7789_cb = {
    .height = ST7789_HEIGHT,
    .width = ST7789_WIDTH
};

const nrf_lcd_t nrf_lcd_st7789 = {
    .lcd_init = st7789_init,
    .lcd_uninit = st7789_uninit,
    .lcd_pixel_draw = st7789_pixel_draw,
    .lcd_rect_draw = st7789_rect_draw,
    .lcd_display = st7789_dummy_display,
    .lcd_rotation_set = st7789_rotation_set,
    .lcd_display_invert = st7789_display_invert,
    .p_lcd_cb = &st7789_cb
};



#endif // NRF_MODULE_ENABLED(ST7789)
