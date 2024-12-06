#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrfx_spim.h"
#include "app_timer.h"
#include "nrf_lcd.h"
#include "math.h"

#include <string.h>

#include "scales_lcd.h"
#include "lvgl/lvgl.h"
#include "ui/ui.h"
#include "coffee_beans.h"
#include "water_droplet.h"
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

    display_sleep();
}

void display_driver_init()
{
    // Set LCD pins as outputs
    nrf_gpio_cfg_output(ST7789_EN_PIN);
    nrf_gpio_cfg_output(ST7789_RST_PIN);
    nrf_gpio_cfg_output(ST7789_BACKLIGHT_PIN);

    display_turn_backlight_off();

    display_power_display_on();
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


    ret_code_t err_code = app_timer_create(&m_lvgl_timer_id, APP_TIMER_MODE_REPEATED, lvgl_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

void display_update_weight_label(float weight)
{
    if (objects.label_weight_integer == NULL)
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
    sprintf(buffer, "%s", wholeNumbers);
    lv_label_set_text( objects.label_weight_integer, buffer );

    char buffer2[2];
    sprintf(buffer2, "%s", fractionalPart);
    lv_label_set_text( objects.label_weight_fraction, fractionalPart );
    }
}

void display_indicate_tare()
{
    if (objects.label_weight_integer == NULL)
    {
        return;
    }
    lv_label_set_text( objects.label_weight_integer, "Taring");
}

void display_update_timer_label(uint32_t seconds)
{
    if (objects.label_timer == NULL)
    {
        return;
    }

    int hours, minutes, remainingSeconds;

    hours = seconds / 3600;
    minutes = (seconds % 3600) / 60;
    remainingSeconds = seconds % 60;

    char buffer[9];
    sprintf(buffer, "%02d:%02d", minutes, remainingSeconds);

    lv_label_set_text( objects.label_timer, buffer);
}

void display_update_battery_label(uint8_t batteryLevel)
{
    if (objects.label_battery_percentage == NULL)
    {
        return;
    }
    char buffer[4];
    sprintf(buffer, "%d%%", batteryLevel);

    lv_label_set_text( objects.label_battery_percentage, buffer);
}

void display_update_coffee_weight_label(float weight)
{
    if (objects.label_coffee_weight == NULL)
    {
        return;
    }

    char buffer[6];

    // Convert float to string
    sprintf(buffer, "%0.1f", weight);

    lv_label_set_text( objects.label_coffee_weight, buffer );
}

void display_update_water_weight_label(float weight)
{
    if (objects.label_water_weight == NULL)
    {
        return;
    }

    char buffer[6];

    // Convert float to string
    sprintf(buffer, "%0.1f", weight);

    lv_label_set_text( objects.label_water_weight, buffer );
}

void display_button1_count_label(uint16_t count)
{


    char buffer[4];

    // Convert float to string
    sprintf(buffer, "%d", count);

    //lv_label_set_text( button1CountLabel, buffer );
}

void display_button2_count_label(uint16_t count)
{

    char buffer[4];

    // Convert float to string
    sprintf(buffer, "%d", count);

    //lv_label_set_text( button2CountLabel, buffer );
}

void display_button3_count_label(uint16_t count)
{


    char buffer[4];

    // Convert float to string
    sprintf(buffer, "%d", count);

    //lv_label_set_text( button3CountLabel, buffer );
}

void display_button4_count_label(uint16_t count)
{


    char buffer[4];

    // Convert float to string
    sprintf(buffer, "%d", count);

    //lv_label_set_text( button4CountLabel, buffer );
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
    // lv_obj_remove_flag(bluetooth_logo_image, LV_OBJ_FLAG_HIDDEN);
}

void display_bluetooth_logo_hide()
{
    // lv_obj_add_flag(bluetooth_logo_image, LV_OBJ_FLAG_HIDDEN);
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
    display_power_display_on();
    lv_obj_clean(lv_scr_act());
    display_lvgl_init();
    ui_init();
    display_driver_init();
    start_lvgl_timer();
    lvgl_timeout_handler(NULL);
    display_turn_backlight_on();
}

void display_reset()
{
    nrf_gpio_pin_clear(ST7789_RST_PIN);
    nrf_delay_ms(100);
    nrf_gpio_pin_set(ST7789_RST_PIN);
}


