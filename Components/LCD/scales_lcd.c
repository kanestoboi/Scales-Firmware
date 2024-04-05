#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrfx_spim.h"
#include "app_timer.h"
#include "nrf_lcd.h"
#include "math.h"

#include <string.h>

#include "scales_lcd.h"
#include "lvgl/lvgl.h"
#include "coffee_beans.h"
#include "water-drops.h"
#include "bluetooth_logo.h"

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
lv_obj_t *weightDecimalLabel;
lv_obj_t *batteryLabel;
lv_obj_t *timeLabel;
lv_obj_t *coffeeWeightLabel;
lv_obj_t *waterWeightLabel;

lv_obj_t *button1CountLabel;
lv_obj_t *button2CountLabel;
lv_obj_t *button3CountLabel;
lv_obj_t *button4CountLabel;

lv_obj_t * bluetooth_logo_image;

#define LVGL_TIMER_INTERVAL_MS              5   // 5ms
#define LVGL_TIMER_INTERVAL_TICKS           APP_TIMER_TICKS(LVGL_TIMER_INTERVAL_MS)
//LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes
#define DRAW_BUF_SIZE (hor_res * ver_res * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE/8];

void display_driver_init();
void display_lvgl_init();


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

void start_lvgl_timer()
{
    ret_code_t err_code = app_timer_start(m_lvgl_timer_id, LVGL_TIMER_INTERVAL_TICKS, NULL);
    APP_ERROR_CHECK(err_code);
}

void stop_lvgl_timer()
{
    ret_code_t err_code = app_timer_stop(m_lvgl_timer_id);
    APP_ERROR_CHECK(err_code);
}

void display_init()
{
    // Initialise LVGL library
    lv_init();

    display = lv_display_create(hor_res, ver_res);


    ret_code_t err_code;

    display_driver_init();

    display_lvgl_init();

    start_lvgl_timer();

    display_turn_backlight_on();
}

void display_driver_init()
{
    // Set LCD pins as outputs
    nrf_gpio_cfg_output(ST7789_EN_PIN);
    nrf_gpio_cfg_output(ST7789_RST_PIN);
    nrf_gpio_cfg_output(ST7789_BACKLIGHT_PIN);

    //display_power_display_on();
    nrf_delay_ms(10);
    display_reset();

    // initialise lcd driver
    ret_code_t err_code = p_lcd->lcd_init();

    if (err_code == NRF_SUCCESS)
    {
        p_lcd->p_lcd_cb->state = NRFX_DRV_STATE_INITIALIZED;
    }

    p_lcd->lcd_rotation_set(NRF_LCD_ROTATE_270);
    p_lcd->lcd_display_invert(true);
}

void display_lvgl_init()
{

    lv_color_t color = {
        .blue = 0xFF,
        .green = 0xFF,
        .red = 0xFF,
    };

    lv_display_set_flush_cb(display, flush_cb);
    lv_display_set_buffers(display, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x0000), LV_PART_MAIN);


    lv_obj_t * coffe_splash_image = lv_image_create(lv_screen_active());
    lv_image_set_src(coffe_splash_image, &coffee_beans);
    lv_obj_align( coffe_splash_image, LV_ALIGN_TOP_LEFT, 0, 0 );

    lv_obj_t * water_drops_image = lv_image_create(lv_screen_active());
    lv_image_set_src(water_drops_image, &water_drops);
    lv_obj_align( water_drops_image, LV_ALIGN_TOP_LEFT, 0, 30 );

    bluetooth_logo_image = lv_image_create(lv_screen_active());
    lv_image_set_src(bluetooth_logo_image, &bluetooth_logo);
    lv_obj_align( bluetooth_logo_image, LV_ALIGN_TOP_RIGHT, -90, 5 );

    display_bluetooth_logo_hide();

    weightLabel = lv_label_create( lv_scr_act() );
    lv_obj_align( weightLabel, LV_ALIGN_BOTTOM_RIGHT, -30, 0 );
    lv_obj_set_style_text_font(weightLabel, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(weightLabel, color, 0);
    lv_label_set_text( weightLabel, "" );

    weightDecimalLabel = lv_label_create( lv_scr_act() );
    lv_obj_align( weightDecimalLabel, LV_ALIGN_BOTTOM_RIGHT, 0, 0 );
    lv_obj_set_style_text_font(weightDecimalLabel, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(weightDecimalLabel, color, 0);
    lv_label_set_text( weightDecimalLabel, "" );

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



    button1CountLabel = lv_label_create( lv_scr_act() );
    lv_obj_align( button1CountLabel, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(button1CountLabel, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(button1CountLabel, color, 0);
    lv_label_set_text( button1CountLabel, "0" );
    
    button2CountLabel = lv_label_create( lv_scr_act() );
    lv_obj_align( button2CountLabel, LV_ALIGN_LEFT_MID, 60, 0);
    lv_obj_set_style_text_font(button2CountLabel, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(button2CountLabel, color, 0);
    lv_label_set_text( button2CountLabel, "0" );

    
    button3CountLabel = lv_label_create( lv_scr_act() );
    lv_obj_align( button3CountLabel, LV_ALIGN_LEFT_MID, 120, 0);
    lv_obj_set_style_text_font(button3CountLabel, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(button3CountLabel, color, 0);
    lv_label_set_text( button3CountLabel, "0" );

    
    button4CountLabel = lv_label_create( lv_scr_act() );
    lv_obj_align( button4CountLabel, LV_ALIGN_LEFT_MID, 180, 0);
    lv_obj_set_style_text_font(button4CountLabel, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(button4CountLabel, color, 0);
    lv_label_set_text( button4CountLabel, "0" );

    ret_code_t err_code = app_timer_create(&m_lvgl_timer_id, APP_TIMER_MODE_REPEATED, lvgl_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

void display_update_weight_label(float weight)
{
    if (weightLabel == NULL)
    {
        return;
    }
    char buffer[5]; // Make sure this buffer is large enough to hold the formatted string

    // Convert float to string
    sprintf(buffer, "%0.1f", weight);

    // Find the position of the decimal point
    char *decimalPointPosition = strchr(buffer, '.');

    if (decimalPointPosition != NULL) {
        // Split the buffer into two parts
        *decimalPointPosition = '\0'; // Terminate the first part

        // First buffer contains the whole numbers before the decimal place
        char wholeNumbers[4];
        strcpy(wholeNumbers, buffer);

        // Second buffer contains the fractional part of the value
        char fractionalPart[2];
        strcpy(fractionalPart, decimalPointPosition + 1);


        // Convert float to string
    sprintf(buffer, "%s.", wholeNumbers);
    lv_label_set_text( weightLabel, buffer );

    char buffer2[2];
    sprintf(buffer2, "%s", fractionalPart);
    lv_label_set_text( weightDecimalLabel, fractionalPart );
    }
}

void display_indicate_tare()
{
    if (weightLabel == NULL)
    {
        return;
    }
    lv_label_set_text( weightLabel, "Taring");
}

void display_update_timer_label(uint32_t seconds)
{
    if (timeLabel == NULL)
    {
        return;
    }

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
    if (batteryLabel == NULL)
    {
        return;
    }
    char buffer[4];
    sprintf(buffer, "%d%%", batteryLevel);

    lv_label_set_text( batteryLabel, buffer);
}

void display_update_coffee_weight_label(float weight)
{
    if (coffeeWeightLabel == NULL)
    {
        return;
    }

    char buffer[6];

    // Convert float to string
    sprintf(buffer, "%0.1f", weight);

    lv_label_set_text( coffeeWeightLabel, buffer );
}

void display_update_water_weight_label(float weight)
{
    if (waterWeightLabel == NULL)
    {
        return;
    }

    char buffer[6];

    // Convert float to string
    sprintf(buffer, "%0.1f", weight);

    lv_label_set_text( waterWeightLabel, buffer );
}

void display_button1_count_label(uint16_t count)
{
    if (button1CountLabel == NULL)
    {
        return;
    }

    char buffer[1];

    // Convert float to string
    sprintf(buffer, "%d", count);

    lv_label_set_text( button1CountLabel, buffer );
}

void display_button2_count_label(uint16_t count)
{
    if (button1CountLabel == NULL)
    {
        return;
    }

    char buffer[1];

    // Convert float to string
    sprintf(buffer, "%d", count);

    lv_label_set_text( button2CountLabel, buffer );
}

void display_button3_count_label(uint16_t count)
{
    if (button1CountLabel == NULL)
    {
        return;
    }

    char buffer[1];

    // Convert float to string
    sprintf(buffer, "%d", count);

    lv_label_set_text( button3CountLabel, buffer );
}

void display_button4_count_label(uint16_t count)
{
    if (button1CountLabel == NULL)
    {
        return;
    }

    char buffer[1];

    // Convert float to string
    sprintf(buffer, "%d", count);

    lv_label_set_text( button4CountLabel, buffer );
}

void display_turn_backlight_on()
{
    nrf_gpio_pin_clear(ST7789_BACKLIGHT_PIN);

}

void display_turn_backlight_off()
{
    nrf_gpio_pin_set(ST7789_BACKLIGHT_PIN);

}

void display_bluetooth_logo_show()
{
    lv_obj_remove_flag(bluetooth_logo_image, LV_OBJ_FLAG_HIDDEN);
}

void display_bluetooth_logo_hide()
{
    lv_obj_add_flag(bluetooth_logo_image, LV_OBJ_FLAG_HIDDEN);
}

void display_power_display_on()
{
    nrf_gpio_pin_clear(ST7789_EN_PIN);
}

void display_power_display_off()
{
    nrf_gpio_pin_set(ST7789_EN_PIN);
}

void display_sleep()
{
    p_lcd->lcd_sleep();
    display_turn_backlight_off();
    display_power_display_off();
    
    stop_lvgl_timer();
}

void display_wakeup()
{
    display_turn_backlight_on();
    display_power_display_on();
    lv_obj_clean(lv_scr_act());
    display_lvgl_init();
    display_driver_init();
    start_lvgl_timer();
}

void display_reset()
{
    nrf_gpio_pin_clear(ST7789_RST_PIN);
    nrf_delay_ms(100);
    nrf_gpio_pin_set(ST7789_RST_PIN);
}


