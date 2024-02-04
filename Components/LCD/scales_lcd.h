#ifndef SCALES_LCD_H__
#define SCALES_LCD_H__

void scales_lcd_init();

void screen_clear(void);
void display_indicate_tare();
void display_update_weight_label(float weight);

void display_update_timer_label(uint32_t seconds);
void display_update_battery_label(uint8_t batteryLevel);
void text_print();

void display_turn_backlight_on();
void display_turn_backlight_off();

#endif