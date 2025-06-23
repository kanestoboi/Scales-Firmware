#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 172);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // label_timer
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_timer = obj;
            lv_obj_set_pos(obj, 0, 115);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "00:00");
        }
        {
            // label_weight_integer
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_weight_integer = obj;
            lv_obj_set_pos(obj, -42, 55);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_RIGHT_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "000");
        }
        {
            // label_weight_decimal
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_weight_decimal = obj;
            lv_obj_set_pos(obj, 278, 115);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, ".");
        }
        {
            // label_weight_fraction
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_weight_fraction = obj;
            lv_obj_set_pos(obj, 289, 115);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "0");
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 0, 115);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_precise_t line_points[] = {
                { 0, 0 },
                { 320, 0 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_line_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // label_coffee_weight
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_coffee_weight = obj;
            lv_obj_set_pos(obj, 95, 71);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "--.-");
        }
        {
            // label_water_weight
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_water_weight = obj;
            lv_obj_set_pos(obj, 175, 71);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "--.-");
        }
        {
            // label_battery_percentage
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_battery_percentage = obj;
            lv_obj_set_pos(obj, -20, -75);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_RIGHT_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "100");
        }
        {
            // label_battery_percentage_symbol
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_battery_percentage_symbol = obj;
            lv_obj_set_pos(obj, 300, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "%");
        }
        {
            // label_coffee_to_water_ratio_numerator
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_coffee_to_water_ratio_numerator = obj;
            lv_obj_set_pos(obj, 142, 35);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "1");
        }
        {
            // label_coffee_to_water_ratio_denominator
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_coffee_to_water_ratio_denominator = obj;
            lv_obj_set_pos(obj, 166, 35);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "16");
        }
        {
            // label_coffee_to_water_ratio_colon
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_coffee_to_water_ratio_colon = obj;
            lv_obj_set_pos(obj, 157, 35);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, ":");
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
}

void create_screen_graph() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.graph = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 172);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(obj, LV_DIR_NONE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // graph_label_timer
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.graph_label_timer = obj;
            lv_obj_set_pos(obj, 0, 115);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "00:00");
        }
        {
            // graph_label_weight_integer
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.graph_label_weight_integer = obj;
            lv_obj_set_pos(obj, -42, 55);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_RIGHT_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "000");
        }
        {
            // graph__weight_decimal
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.graph__weight_decimal = obj;
            lv_obj_set_pos(obj, 278, 115);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, ".");
        }
        {
            // graph_label_weight_fraction
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.graph_label_weight_fraction = obj;
            lv_obj_set_pos(obj, 289, 115);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffcacaca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "0");
        }
        {
            // weight_chart
            lv_obj_t *obj = lv_chart_create(parent_obj);
            objects.weight_chart = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 320, 115);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_SCROLLED);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_line_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_image_recolor(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_graph();
}

void tick_screen_graph() {
}



typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_graph,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main();
    create_screen_graph();
}
