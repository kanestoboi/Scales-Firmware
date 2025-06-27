#ifndef SCALES_LCD_H__
#define SCALES_LCD_H__

#include "lvgl/lvgl.h"

typedef struct Scales_Display_t
{
    const uint8_t dc_pin;
    const uint8_t rst_pin;
    const uint8_t en_pin;
    const uint8_t backlight_pin;

    const uint16_t height;
    const uint16_t width;

    const uint16_t x_start_offset;
    const uint16_t y_start_offset;

    const nrfx_spim_t * spim_instance;

} Scales_Display_t;


void display_init();

void display_screen_clear(void);
void display_update_weight_label(float weight);

void display_update_coffee_weight_label(float weight);
void display_update_water_weight_label(float weight);

void display_update_timer_label(uint32_t seconds);
void display_update_battery_label(uint8_t batteryLevel);
void display_update_battery_time_to_charge_value(float timeToCharge);
void display_update_battery_time_to_empty_value(float timeToEmpty);
void display_update_battery_cycles_value(float cycles);
void display_update_battery_average_current_value(float averageCurrent);
void display_update_battery_cell_voltage_value(float cellVoltage);
void display_update_battery_full_capacity_value(float fullCapacity);
void display_update_battery_remaining_capacity_value(float remainingCapacity);

void display_update_tare_attempts_label(uint32_t attempts);

void display_turn_backlight_on();
void display_turn_backlight_off();

void display_bluetooth_logo_show();
void display_bluetooth_logo_hide();

void display_power_display_on();
void display_power_display_off();

void display_reset();

void display_sleep();
void display_wakeup();

void display_flash_elapsed_time_label();
void display_stop_flash_elapsed_time_label();
void display_cycle_screen();

void display_spi_xfer_complete_callback(nrfx_spim_evt_t const * p_event, void * p_context);

void display_loop();
#endif