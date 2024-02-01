#include "nrf_gfx.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrfx_spim.h"
#include "app_timer.h"

#include <string.h>

#include "scales_lcd.h"
#include "lvgl/lvgl.h"

extern const nrf_gfx_font_desc_t orkney_8ptFontInfo;
extern const nrf_gfx_font_desc_t orkney_24ptFontInfo;
extern const nrf_lcd_t nrf_lcd_st7735;

APP_TIMER_DEF(m_lvgl_timer_id);

#define hor_res 160
#define ver_res 80
lv_display_t * display;

static const nrf_lcd_t * p_lcd = &nrf_lcd_st7735;

static const uint8_t ST7735_DC_Pin = 16;
static const uint8_t ST7735_SCK_PIN = 14;
static const uint8_t ST7735_MISO_PIN = 12;
static const uint8_t ST7735_MOSI_PIN = 12;
static const uint8_t ST7735_SS_PIN = 17;
static const uint8_t ST7735_RST_PIN = 15;
static const uint8_t ST7735_BACKLIGHT_PIN = 45;

lv_obj_t *weightLabel;

#define LVGL_TIMER_INTERVAL_MS              5   // 5ms
#define LVGL_TIMER_INTERVAL_TICKS           APP_TIMER_TICKS(LVGL_TIMER_INTERVAL_MS)
//LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes
#define DRAW_BUF_SIZE (hor_res * ver_res / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE/8];

static void lvgl_timeout_handler(void * p_context)
{
    lv_task_handler(); // let the GUI do its work 
    lv_tick_inc(LVGL_TIMER_INTERVAL_MS); // tell LVGL how much time has passed 

    ret_code_t err_code = app_timer_start(m_lvgl_timer_id, LVGL_TIMER_INTERVAL_TICKS, NULL);
    APP_ERROR_CHECK(err_code);
}


void flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map)
{
    //The most simple case (but also the slowest) to put all pixels to the screen one-by-one
    //`put_px` is just an example, it needs to be implemented by you.
    uint16_t * buf16 = (uint16_t *)px_map; // Let's say it's a 16 bit (RGB565) display

    uint16_t length = ((area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1))*2;
    lv_draw_sw_rgb565_swap(px_map, length/2);
    
    //p_lcd->lcd_display(0, 0xFFFF, area->x1, area->y1, area->x2, area->y2);
    p_lcd->lcd_display(px_map, length, area->x1, area->y1, area->x2, area->y2);
    // IMPORTANT!!!
    // Inform LVGL that you are ready with the flushing and buf is not used anymore
    lv_display_flush_ready(display);
}

void scales_lcd_init()
{

    ret_code_t err_code;
    // Initialise LVGL library
    lv_init();

    display = lv_display_create(hor_res, ver_res);

        // Reset LCD
    nrf_gpio_cfg_output(ST7735_RST_PIN);
    nrf_gpio_pin_clear(ST7735_RST_PIN);
    nrf_delay_ms(100);
    nrf_gpio_pin_set(ST7735_RST_PIN);

    // set LCD backlight to on
    nrf_gpio_cfg_output(ST7735_BACKLIGHT_PIN);
    nrf_gpio_pin_set(ST7735_BACKLIGHT_PIN);

    // initialise lcd driver
    err_code = p_lcd->lcd_init();
    p_lcd->lcd_rotation_set(NRF_LCD_ROTATE_90);

    if (err_code == NRF_SUCCESS)
    {
        p_lcd->p_lcd_cb->state = NRFX_DRV_STATE_INITIALIZED;
    }

    lv_display_set_flush_cb(display, flush_cb);
    lv_display_set_buffers(display, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);


    weightLabel = lv_label_create( lv_scr_act() );
    lv_obj_align( weightLabel, LV_ALIGN_CENTER, 0, 0 );


    err_code = app_timer_create(&m_lvgl_timer_id, APP_TIMER_MODE_SINGLE_SHOT, lvgl_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_lvgl_timer_id, LVGL_TIMER_INTERVAL_TICKS, NULL);
    APP_ERROR_CHECK(err_code);
    



}


void weight_print(float weight)
{
    char buffer[20];//[20];  // Adjust the buffer size based on your needs

    // Convert float to string
    sprintf(buffer, "%0.1f", weight);

    lv_label_set_text( weightLabel, buffer );
}

void text_print(void)
{
    lv_label_set_text( weightLabel, "Scales" );
}

void print_taring()
{
    lv_label_set_text( weightLabel, "Taring");
}

void printSquare()
{
    nrf_gfx_print_square(p_lcd);
}