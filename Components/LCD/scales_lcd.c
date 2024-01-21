#include "nrf_gfx.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include <string.h>
#include "scales_lcd.h"

extern const nrf_gfx_font_desc_t orkney_8ptFontInfo;
extern const nrf_gfx_font_desc_t orkney_24ptFontInfo;
extern const nrf_lcd_t nrf_lcd_st7735;

static const nrf_lcd_t * p_lcd = &nrf_lcd_st7735;
static const nrf_gfx_font_desc_t * p_font = &orkney_24ptFontInfo;

static const uint8_t ST7735_DC_Pin = 16;
static const uint8_t ST7735_SCK_PIN = 14;
static const uint8_t ST7735_MISO_PIN = 12;
static const uint8_t ST7735_MOSI_PIN = 12;
static const uint8_t ST7735_SS_PIN = 17;
static const uint8_t ST7735_RST_PIN = 15;
static const uint8_t ST7735_BACKLIGHT_PIN = 45;

char m_last_weight_string[20] = "";

void scales_lcd_init()
{
    // Reset LCD
    nrf_gpio_cfg_output(ST7735_RST_PIN);
    nrf_gpio_pin_clear(ST7735_RST_PIN);
    nrf_delay_ms(100);
    nrf_gpio_pin_set(ST7735_RST_PIN);

    nrf_gfx_init(p_lcd);

    nrf_gfx_rotation_set(p_lcd, NRF_LCD_ROTATE_90);

    screen_clear();

    // set LCD backlight to on
    nrf_gpio_cfg_output(ST7735_BACKLIGHT_PIN);
    nrf_gpio_pin_set(ST7735_BACKLIGHT_PIN);

}


void screen_clear(void)
{
    nrf_gfx_screen_fill(p_lcd, 0xFFFF);
}

void weight_print(float weight)
{
    char buffer[20];//[20];  // Adjust the buffer size based on your needs

    // Convert float to string
    sprintf(buffer, "%0.1f", weight);


    nrf_gfx_point_t text_start2 = NRF_GFX_POINT(5, nrf_gfx_height_get(p_lcd)/2-20);
    APP_ERROR_CHECK(nrf_gfx_print_fast(p_lcd, &text_start2, 0xFFFF, 0x00, buffer, &orkney_24ptFontInfo, true));
    memcpy(m_last_weight_string, buffer, 20);
}

void text_print(void)
{
    static const char * test_text = "Scales";
    nrf_gfx_point_t text_start = NRF_GFX_POINT(5, nrf_gfx_height_get(p_lcd)/2-20);
    APP_ERROR_CHECK(nrf_gfx_print_fast(p_lcd, &text_start, 0xFFFF, 0, test_text, p_font, true));
}

void print_taring()
{
    static const char * test_text = "Taring";
    nrf_gfx_point_t text_start = NRF_GFX_POINT(5, nrf_gfx_height_get(p_lcd)/2-20);
    APP_ERROR_CHECK(nrf_gfx_print_fast(p_lcd, &text_start, 0xFFFF, 0, test_text, p_font, true));
}

void printSquare()
{
    nrf_gfx_print_square(p_lcd);
}