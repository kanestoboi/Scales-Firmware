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

#ifndef NRF_LCD_H__
#define NRF_LCD_H__

#include <nrfx.h>
#include "nrfx_spim.h"

/** @file
 *
 * @defgroup nrf_lcd LCD Library
 * @{
 * @ingroup nrf_gfx
 *
 * @brief This module defines generic API for LCDs.
 */

/**
 * @brief Enumerator with available rotations.
 */
typedef enum{
    NRF_LCD_ROTATE_0 = 0,       /**< Rotate 0 degrees, clockwise. */
    NRF_LCD_ROTATE_90,          /**< Rotate 90 degrees, clockwise. */
    NRF_LCD_ROTATE_180,         /**< Rotate 180 degrees, clockwise. */
    NRF_LCD_ROTATE_270          /**< Rotate 270 degrees, clockwise. */
}nrf_lcd_rotation_t;

/**
 * @brief LCD instance control block.
 */
typedef struct
{
    uint8_t dc_pin;
    uint8_t sck_pin;
    uint8_t miso_pin;
    uint8_t mosi_pin;
    uint8_t ss_pin;
    uint8_t rst_pin;
    uint8_t en_pin;
    uint8_t backlight_pin;
}lcd_pins_t;

/**
 * @brief LCD instance control block.
 */
typedef struct
{
    nrfx_drv_state_t state;         /**< State of LCD instance. */
    uint16_t height;                /**< LCD height. */
    uint16_t width;                 /**< LCD width. */
    nrf_lcd_rotation_t rotation;    /**< LCD rotation. */
    const nrfx_spim_t * spim_instance;
    lcd_pins_t pins;
}lcd_cb_t;

/**
 * @brief LCD instance type.
 *
 * This structure provides generic API for LCDs.
 */
typedef struct
{
    /**
     * @brief Function for initializing the LCD controller.
     */
    ret_code_t (* lcd_init)(const nrfx_spim_t * spim, uint8_t dc_pin);

    /**
     * @brief Function for uninitializing the LCD controller.
     */
    void (* lcd_uninit)(void);

    /**
     * @brief Function for displaying data from an internal frame buffer.
     *
     * This function may be used when functions for drawing do not write directly to
     * LCD but to an internal frame buffer. It could be implemented to write data from this
     * buffer to LCD.
    */
    nrfx_err_t (* lcd_display)(const nrfx_spim_t * spim, uint8_t dc_pin, uint8_t * data, uint16_t len, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    /**
     * @brief Function for rotating the screen.
     *
     * @param[in] rotation      Rotation as enumerated value.
     */
    void (* lcd_rotation_set)(const nrfx_spim_t * spim, uint8_t dc_pin, nrf_lcd_rotation_t rotation);

    /**
     * @brief Function for setting inversion of colors on the screen.
     *
     * @param[in] invert        If true, inversion will be set.
     */
    void (* lcd_display_invert)(const nrfx_spim_t * spim, uint8_t dc_pin, bool invert);

    nrfx_err_t (* lcd_set_addr_window_to_buffer)(const nrfx_spim_t * spim, uint8_t dc_pin, uint8_t * data, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

    void (* xfer_complete_handler)(const nrfx_spim_t * spim, uint8_t dc_pin);

    /**
     * @brief Pointer to the LCD instance control block.
     */
    lcd_cb_t * p_lcd_cb;
}nrf_lcd_t;


/* @} */

#endif // NRF_LCD_H__
