#ifndef SCALES_LCD_H__
#define SCALES_LCD_H__

void scales_lcd_init();

void screen_clear(void);
void print_taring();
void weight_print(float weight);
void text_print();

#endif