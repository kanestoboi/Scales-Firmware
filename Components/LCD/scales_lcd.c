#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrfx_spim.h"
#include "app_timer.h"
#include "nrf_lcd.h"
#include "math.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

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
APP_TIMER_DEF(m_elapsed_time_timer_id);

lv_display_t * p_lv_display1;
lv_display_t * flush_cb_display = NULL;
uint32_t flush_length = 0;
lv_area_t flush_cb_area;
enum ScreensEnum  current_screen_id = SCREEN_ID_MAIN;

Scales_Display_t * p_scales_display1;

static const nrf_lcd_t * p_nrf_lcd_driver = &nrf_lcd_st7789;

#define hor_res 320
#define ver_res 172

lv_obj_t * bluetooth_logo_image;
static bool flash_timer_label = false;

static lv_chart_series_t * mWeightChartSeries;  // Keep series global if needed later
static lv_chart_series_t * mTargetWaterWeightSeries;

char mBatteryLevelCharacterBuffer[4];
char wholeNumbers[5];
char fractionalPart[2];

bool mWeightUpdated = false;
bool mToggleElapsedTimeVisibility = false;
bool mCoffeeWeightUpdated = false;
bool mWaterWeightUpdated = false;
bool mTimerValueUpdated = false;
bool mClearTimerFlashing = false;
bool mBatteryLevelUpdated = false;

float mWeight = 0.0;
float mCoffeeWeight = 0.0;
float mWaterWeight = 0.0;
uint32_t mTimerValue = 0;
uint8_t mBatteryLevel = 0;

#define LVGL_TIMER_INTERVAL_MS              5   // 5ms
#define LVGL_TIMER_INTERVAL_TICKS           APP_TIMER_TICKS(LVGL_TIMER_INTERVAL_MS)

#define ELAPSED_TIME_TIMER_INTERVAL_MS              1000   // 1000ms
#define ELAPSED_TIME_TIMER_INTERVAL_TICKS           APP_TIMER_TICKS(ELAPSED_TIME_TIMER_INTERVAL_MS)

//LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes
#define DRAW_BUF_SIZE (hor_res * ver_res * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE/8];

void display_driver_init();

void display_toggle_timer_label_visibility()
{
    if (objects.label_timer == NULL)
        return;

    if (lv_obj_has_flag(objects.label_timer, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(objects.graph_label_timer, LV_OBJ_FLAG_HIDDEN); // Show
        lv_obj_clear_flag(objects.label_timer, LV_OBJ_FLAG_HIDDEN); // Show
    } else {
        lv_obj_add_flag(objects.label_timer, LV_OBJ_FLAG_HIDDEN);   // Hide
        lv_obj_add_flag(objects.graph_label_timer, LV_OBJ_FLAG_HIDDEN);   // Hide
    }
}

void display_flash_elapsed_time_label()
{
    ret_code_t err_code = app_timer_start(m_elapsed_time_timer_id, ELAPSED_TIME_TIMER_INTERVAL_TICKS, NULL);
    APP_ERROR_CHECK(err_code);
}

void display_stop_flash_elapsed_time_label()
{
    mClearTimerFlashing = true;
    ret_code_t err_code = app_timer_stop(m_elapsed_time_timer_id);
    APP_ERROR_CHECK(err_code);
}

static void lvgl_timeout_handler(void * p_context)
{
    lv_tick_inc(LVGL_TIMER_INTERVAL_MS); // tell LVGL how much time has passed 
}

static void elapsed_time_timeout_handler(void * p_context)
{
    mToggleElapsedTimeVisibility = true;
}

void flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map)
{
    uint16_t * buf16 = (uint16_t *)px_map; // 16 bit (RGB565) display. cast pixel map to uint16_t pointer

    // length is area to write (in pixels) multiplied by 2 since each pixel is 2 bytes
    uint16_t length = ((area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1)) * 2; 

    flush_cb_display = display;
    flush_length = length;
    
    nrfx_err_t err_code = p_nrf_lcd_driver->lcd_display(p_scales_display1->spim_instance, p_scales_display1->dc_pin, px_map, length, area->x1, area->y1, area->x2, area->y2);
    
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Error flushing cb");
        NRF_LOG_FLUSH();
        flush_cb_display = NULL;
        flush_length = 0;
        lv_display_flush_ready(display);
    }
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

void display_init(Scales_Display_t * scales_display)
{
    // Initialise LVGL library
    lv_init();

    p_lv_display1 = lv_display_create(hor_res, ver_res);
    
    p_scales_display1 = scales_display;

    lv_display_set_flush_cb(p_lv_display1, flush_cb);
    lv_display_set_buffers(p_lv_display1, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x0000), LV_PART_MAIN);

    // Set LCD pins as outputs
    nrf_gpio_cfg_output(p_scales_display1->en_pin);
    nrf_gpio_cfg_output(p_scales_display1->rst_pin);
    nrf_gpio_cfg_output(p_scales_display1->backlight_pin);
    nrf_gpio_cfg_output(p_scales_display1->dc_pin);

    ret_code_t err_code = app_timer_create(&m_lvgl_timer_id, APP_TIMER_MODE_REPEATED, lvgl_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_elapsed_time_timer_id, APP_TIMER_MODE_REPEATED, elapsed_time_timeout_handler);
    APP_ERROR_CHECK(err_code);

    display_sleep();
}

void display_driver_init()
{
    // initialise lcd driver
    ret_code_t err_code = p_nrf_lcd_driver->lcd_init(p_scales_display1->spim_instance, p_scales_display1->dc_pin);

    if (err_code == NRF_SUCCESS)
    {
        p_nrf_lcd_driver->p_lcd_cb->state = NRFX_DRV_STATE_INITIALIZED;
    }
}


void display_update_weight_label(float weight)
{
    mWeight = weight;
    char buffer[5]; // Make sure this buffer is large enough to hold the formatted string

    // Convert float to string
    sprintf(buffer, "%0.1f", weight);

    // Find the position of the decimal point
    char *decimalPointPosition = strchr(buffer, '.');

    if (decimalPointPosition != NULL) {
        // Split the buffer into two parts
        *decimalPointPosition = '\0'; // Terminate the first part

        // First buffer contains the whole numbers before the decimal place
        strcpy(wholeNumbers, buffer);

        // Second buffer contains the fractional part of the value
        strcpy(fractionalPart, decimalPointPosition + 1);

        char buffer2[2];
        sprintf(buffer2, "%s", fractionalPart);

        mWeightUpdated = true;

    }
}

void display_update_timer_label(uint32_t seconds)
{
    if (objects.label_timer == NULL)
    {
        return;
    }

    mTimerValue = seconds;
    mTimerValueUpdated = true;
}

void display_update_battery_label(uint8_t batteryLevel)
{
    if (objects.label_battery_percentage == NULL)
    {
        return;
    }

    mBatteryLevel = batteryLevel;
    mBatteryLevelUpdated = true;
}

void display_update_coffee_weight_label(float weight)
{
    if (objects.label_coffee_weight == NULL)
    {
        return;
    }

    mCoffeeWeight = weight;
    mCoffeeWeightUpdated = true;
}

void display_update_water_weight_label(float weight)
{
    if (objects.label_water_weight == NULL)
    {
        return;
    }

    mWaterWeight = weight;
    mWaterWeightUpdated = true;
}

void display_turn_backlight_on()
{
    nrf_gpio_pin_clear(p_scales_display1->backlight_pin);

}

void display_turn_backlight_off()
{
    nrf_gpio_pin_set(p_scales_display1->backlight_pin);

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
    nrf_gpio_pin_clear(p_scales_display1->en_pin);
}

void display_power_display_off()
{
    nrf_gpio_pin_set(p_scales_display1->en_pin);
}

void display_sleep()
{
    p_nrf_lcd_driver->lcd_uninit();
    stop_lvgl_timer();
    display_turn_backlight_off();
    display_power_display_off();
    

    nrf_gpio_cfg_default(p_scales_display1->en_pin);
    nrf_gpio_cfg_default(p_scales_display1->rst_pin);
    nrf_gpio_cfg_default(p_scales_display1->backlight_pin);
    nrf_gpio_cfg_default(p_scales_display1->dc_pin);
}

void display_wakeup()
{
    nrf_gpio_cfg_output(p_scales_display1->en_pin);
    nrf_gpio_cfg_output(p_scales_display1->rst_pin);
    nrf_gpio_cfg_output(p_scales_display1->backlight_pin);
    nrf_gpio_cfg_output(p_scales_display1->dc_pin);

    display_power_display_on();

    lv_obj_clean(lv_scr_act());

    display_reset();

    ui_init();
    display_chart_init();
        
    display_driver_init();

    start_lvgl_timer();

    display_turn_backlight_on();
}

void display_reset()
{
    nrf_gpio_pin_clear(p_scales_display1->rst_pin);
    nrf_delay_ms(100);
    nrf_gpio_pin_set(p_scales_display1->rst_pin);
}

void display_chart_init()
{  
    // Set chart type

}

void display_cycle_screen()
{
    if (current_screen_id != SCREEN_ID_GRAPH)
    {
        loadScreen(SCREEN_ID_GRAPH);
        current_screen_id = SCREEN_ID_GRAPH;
    }
    else
    {
        loadScreen(SCREEN_ID_MAIN);
        current_screen_id = SCREEN_ID_MAIN;
    }
}

void display_spi_xfer_complete_callback(nrfx_spim_evt_t const * p_event, void * p_context)
{
    if (p_event->type == NRFX_SPIM_EVENT_DONE)
    {
        if (flush_cb_display != NULL && p_event->xfer_desc.tx_length == flush_length) {
            lv_display_flush_ready(flush_cb_display);
            flush_cb_display = NULL;
        }
        p_nrf_lcd_driver->xfer_complete_handler(p_scales_display1->spim_instance, p_scales_display1->dc_pin);
    }
}

void display_loop()
{
    lv_task_handler(); // let the GUI do its work   
    
    //lv_label_set_text( objects.label_weight_integer, buffer );
    if (mWeightUpdated)
    {   
        lv_label_set_text(objects.label_weight_integer, wholeNumbers);
        lv_label_set_text(objects.label_weight_fraction, fractionalPart);

        lv_label_set_text(objects.graph_label_weight_integer, wholeNumbers);
        lv_label_set_text(objects.graph_label_weight_fraction, fractionalPart);

        lv_bar_set_value(objects.graph_bar, mWeight, LV_ANIM_ON);

        if (mWeight >= mWaterWeight - 5.0 ) 
        {
            // Set to green
            lv_obj_set_style_bg_color(objects.graph_bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        } 
        else if (mWeight >= mWaterWeight - mWaterWeight*0.1)
        {
            // Set to Yellow
            lv_obj_set_style_bg_color(objects.graph_bar, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        }
        else
        {
            // Set to red
            lv_obj_set_style_bg_color(objects.graph_bar, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        }

        mWeightUpdated = false;
    }
    
    if (mToggleElapsedTimeVisibility)
    {
        display_toggle_timer_label_visibility();
        mToggleElapsedTimeVisibility = false;
    } 

    if (mWaterWeightUpdated)
    {
        char buffer[6];

        // Convert float to string
        sprintf(buffer, "%0.1f", mWaterWeight);

        lv_label_set_text( objects.label_water_weight, buffer );


        lv_bar_set_range(objects.graph_bar, 0, mWaterWeight);
        mWaterWeightUpdated = false;
    }

    if (mCoffeeWeightUpdated)
    {
        char buffer[6];

        // Convert float to string
        sprintf(buffer, "%0.1f", mCoffeeWeight);

        lv_label_set_text( objects.label_coffee_weight, buffer);

        mCoffeeWeightUpdated = false;
    }

    if (mTimerValueUpdated)
    {
        int hours, minutes, remainingSeconds;

        hours = mTimerValue / 3600;
        minutes = (mTimerValue % 3600) / 60;
        remainingSeconds = mTimerValue % 60;

        char buffer[9];
        sprintf(buffer, "%02d:%02d", minutes, remainingSeconds);

        lv_label_set_text( objects.label_timer, buffer);
        lv_label_set_text( objects.graph_label_timer, buffer);
        mTimerValueUpdated = false;
    }

    if (mClearTimerFlashing)
    {
        lv_obj_clear_flag(objects.label_timer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(objects.graph_label_timer, LV_OBJ_FLAG_HIDDEN);
        mClearTimerFlashing = false;
    }

    if (mBatteryLevelUpdated)
    {
        sprintf(mBatteryLevelCharacterBuffer, "%d\%", mBatteryLevel);

        lv_label_set_text( objects.label_battery_percentage, mBatteryLevelCharacterBuffer);
        mBatteryLevelUpdated = false;
    }
}