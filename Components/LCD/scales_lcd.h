#ifndef SCALES_LCD_H__
#define SCALES_LCD_H__

void display_init();

void screen_clear(void);
void display_indicate_tare();
void display_update_weight_label(float weight);

void display_update_coffee_weight_label(float weight);
void display_update_water_weight_label(float weight);

void display_button1_count_label(uint16_t count);
void display_button2_count_label(uint16_t count);
void display_button3_count_label(uint16_t count);
void display_button4_count_label(uint16_t count);

void display_button1_threshold_label(uint16_t threshold);
void display_button2_threshold_label(uint16_t threshold);
void display_button3_threshold_label(uint16_t threshold);
void display_button4_threshold_label(uint16_t threshold);

void display_update_timer_label(uint32_t seconds);
void display_update_battery_label(uint8_t batteryLevel);

void text_print();

void display_turn_backlight_on();
void display_turn_backlight_off();

void display_bluetooth_logo_show();
void display_bluetooth_logo_hide();

void display_power_display_on();
void display_power_display_off();

void display_reset();

void display_sleep();
void display_wakeup();

#endif