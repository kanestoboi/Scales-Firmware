#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrfx_spim.h"
#include "app_timer.h"
#include "nrf_lcd.h"

#include <string.h>

#include "scales_lcd.h"
#include "lvgl/lvgl.h"
#include "coffee_beans.h"
#include "water-drops.h"

extern const nrf_lcd_t nrf_lcd_st7735;
extern const nrf_lcd_t nrf_lcd_st7789;

APP_TIMER_DEF(m_lvgl_timer_id);

#define hor_res 320
#define ver_res 172
lv_display_t * display;

static const nrf_lcd_t * p_lcd = &nrf_lcd_st7789;

static const uint8_t ST7735_DC_Pin = 16;
static const uint8_t ST7735_SCK_PIN = 14;
static const uint8_t ST7735_MISO_PIN = 12;
static const uint8_t ST7735_MOSI_PIN = 12;
static const uint8_t ST7735_SS_PIN = 17;
static const uint8_t ST7735_RST_PIN = 15;
static const uint8_t ST7735_BACKLIGHT_PIN = 7;


static const uint8_t ST7789_DC_PIN = 40;
static const uint8_t ST7789_SCK_PIN = 12;
static const uint8_t ST7789_MISO_PIN = 14;
static const uint8_t ST7789_MOSI_PIN = 14;
static const uint8_t ST7789_SS_PIN = 11;
static const uint8_t ST7789_RST_PIN = 16;
static const uint8_t ST7789_EN_PIN = 41;
static const uint8_t ST7789_BACKLIGHT_PIN = 7;


lv_obj_t *weightLabel;
lv_obj_t *batteryLabel;
lv_obj_t *timeLabel;
lv_obj_t *coffeeWeightLabel;
lv_obj_t *waterWeightLabel;

#define LVGL_TIMER_INTERVAL_MS              5   // 5ms
#define LVGL_TIMER_INTERVAL_TICKS           APP_TIMER_TICKS(LVGL_TIMER_INTERVAL_MS)
//LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes
#define DRAW_BUF_SIZE (hor_res * ver_res * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE/8];

static void lvgl_timeout_handler(void * p_context)
{
    lv_task_handler(); // let the GUI do its work 
    lv_tick_inc(LVGL_TIMER_INTERVAL_MS); // tell LVGL how much time has passed 
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
/*    nrf_gpio_cfg_output(ST7735_RST_PIN);
    nrf_gpio_pin_clear(ST7735_RST_PIN);
    nrf_delay_ms(100);
    nrf_gpio_pin_set(ST7735_RST_PIN);
    // set LCD backlight pin to output
    nrf_gpio_cfg_output(ST7735_BACKLIGHT_PIN);
*/
    nrf_gpio_cfg_output(ST7789_EN_PIN);
    nrf_gpio_pin_clear(ST7789_EN_PIN);
    nrf_delay_ms(10);
    nrf_gpio_cfg_output(ST7789_RST_PIN);
    nrf_gpio_pin_clear(ST7789_RST_PIN);
    nrf_delay_ms(100);
    nrf_gpio_pin_set(ST7789_RST_PIN);
    // set LCD backlight pin to output
    nrf_gpio_cfg_output(ST7789_BACKLIGHT_PIN);


    // initialise lcd driver
    err_code = p_lcd->lcd_init();
    p_lcd->lcd_rotation_set(NRF_LCD_ROTATE_270);

    if (err_code == NRF_SUCCESS)
    {
        p_lcd->p_lcd_cb->state = NRFX_DRV_STATE_INITIALIZED;
    }

    p_lcd->lcd_display_invert(true);

    lv_color_t color = {
        .blue = 0xFF,
        .green = 0xFF,
        .red = 0xFF,
    };

    lv_display_set_flush_cb(display, flush_cb);
    lv_display_set_buffers(display, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x0000), LV_PART_MAIN);

    lv_obj_t * coffe_splash_image = lv_image_create(lv_screen_active());

    /*From variable*/
    lv_image_set_src(coffe_splash_image, &coffee_beans);
    lv_obj_align( coffe_splash_image, LV_ALIGN_TOP_LEFT, 0, 0 );

    lv_obj_t * water_drops_image = lv_image_create(lv_screen_active());

    /*From variable*/
    lv_image_set_src(water_drops_image, &water_drops);
    lv_obj_align( water_drops_image, LV_ALIGN_TOP_LEFT, 0, 30 );

    //lv_obj_add_flag(coffe_splash_image, LV_OBJ_FLAG_HIDDEN);

    weightLabel = lv_label_create( lv_scr_act() );
    lv_obj_align( weightLabel, LV_ALIGN_BOTTOM_RIGHT, 0, 0 );
    lv_obj_set_style_text_font(weightLabel, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(weightLabel, color, 0);
    lv_label_set_text( weightLabel, "" );

    batteryLabel = lv_label_create( lv_scr_act() );
    lv_obj_align( batteryLabel, LV_ALIGN_TOP_RIGHT, -8, 0 );
    lv_obj_set_style_text_font(batteryLabel, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(batteryLabel, color, 0);
    lv_label_set_text( batteryLabel, "" );

    timeLabel = lv_label_create( lv_scr_act() );
    lv_obj_align( timeLabel, LV_ALIGN_BOTTOM_LEFT, 0, 0 );
    lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(timeLabel, color, 0);
    lv_label_set_text( timeLabel, "00:00" );

    coffeeWeightLabel = lv_label_create( lv_scr_act() );
    lv_obj_align( coffeeWeightLabel, LV_ALIGN_TOP_LEFT, 55, 0 );
    lv_obj_set_style_text_font(coffeeWeightLabel, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(coffeeWeightLabel, color, 0);
    lv_label_set_text( coffeeWeightLabel, "0.0" );

    waterWeightLabel = lv_label_create( lv_scr_act() );
    lv_obj_align( waterWeightLabel, LV_ALIGN_TOP_LEFT, 55, 30 );
    lv_obj_set_style_text_font(waterWeightLabel, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(waterWeightLabel, color, 0);
    lv_label_set_text( waterWeightLabel, "0.0" );

    err_code = app_timer_create(&m_lvgl_timer_id, APP_TIMER_MODE_REPEATED, lvgl_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_lvgl_timer_id, LVGL_TIMER_INTERVAL_TICKS, NULL);
    APP_ERROR_CHECK(err_code);

    display_turn_backlight_on();
}


void display_update_weight_label(float weight)
{
    char buffer[6];

    // Convert float to string
    sprintf(buffer, "%0.1f", weight);

    lv_label_set_text( weightLabel, buffer );
}

void text_print(void)
{
    lv_label_set_text( weightLabel, "Scales" );
}

void display_indicate_tare()
{
    lv_label_set_text( weightLabel, "Taring");
}

void display_update_timer_label(uint32_t seconds)
{
    int hours, minutes, remainingSeconds;

    hours = seconds / 3600;
    minutes = (seconds % 3600) / 60;
    remainingSeconds = seconds % 60;

    char buffer[9];
    sprintf(buffer, "%02d:%02d", minutes, remainingSeconds);

    lv_label_set_text( timeLabel, buffer);
}

void display_update_battery_label(uint8_t batteryLevel)
{
    char buffer[4];
    sprintf(buffer, "%d%%", batteryLevel);

    lv_label_set_text( batteryLabel, buffer);
}

void display_update_coffee_weight_label(float weight)
{
    char buffer[6];

    // Convert float to string
    sprintf(buffer, "%0.1f", weight);

    lv_label_set_text( coffeeWeightLabel, buffer );
}

void display_update_water_weight_label(float weight)
{
    char buffer[6];

    // Convert float to string
    sprintf(buffer, "%0.1f", weight);

    lv_label_set_text( waterWeightLabel, buffer );
}

void display_turn_backlight_on()
{
    nrf_gpio_pin_clear(ST7789_BACKLIGHT_PIN);

}

void display_turn_backlight_off()
{
    nrf_gpio_pin_set(ST7789_BACKLIGHT_PIN);

}