#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include "../lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *diagnostics;
    lv_obj_t *label_timer;
    lv_obj_t *label_weight_integer;
    lv_obj_t *label_weight_decimal;
    lv_obj_t *label_weight_fraction;
    lv_obj_t *obj0;
    lv_obj_t *label_coffee_weight;
    lv_obj_t *label_water_weight;
    lv_obj_t *label_battery_percentage;
    lv_obj_t *label_coffee_to_water_ratio_numerator;
    lv_obj_t *label_coffee_to_water_ratio_denominator;
    lv_obj_t *label_coffee_to_water_ratio_colon;
    lv_obj_t *graph_bar;
    lv_obj_t *diagnostics_charge_label;
    lv_obj_t *diagnostics_charge_value;
    lv_obj_t *diagnostics1_time_to_charge_label;
    lv_obj_t *diagnostics_time_to_charge_value;
    lv_obj_t *diagnostics1_time_to_empty_label;
    lv_obj_t *diagnostics_time_to_empty_value;
    lv_obj_t *diagnostics1_cycles_label_label;
    lv_obj_t *diagnostics_cycles_value;
    lv_obj_t *diagnostics1_average_current_label;
    lv_obj_t *diagnostics_average_current_value;
    lv_obj_t *diagnostics1_cell_voltage_label;
    lv_obj_t *diagnostics_cell_voltage_value;
    lv_obj_t *diagnostics_battery_information_label;
    lv_obj_t *diagnostics1_full_capacity_label;
    lv_obj_t *diagnostics_full_capacity_value;
    lv_obj_t *diagnostics1_remaining_capacity_label;
    lv_obj_t *diagnostics_remaining_capacity_value;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_DIAGNOSTICS = 2,
};

void create_screen_main();
void tick_screen_main();

void create_screen_diagnostics();
void tick_screen_diagnostics();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/