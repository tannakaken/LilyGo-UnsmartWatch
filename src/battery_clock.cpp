#include <LilyGoLib.h>
#include <LV_Helper.h>
#include "common.hpp"
#include "battery_clock.hpp"

extern lv_obj_t *canvas;
extern lv_color_t bg_color;

const int OUTER_RADIUS = 80;
const int ARC_WIDTH = 30;
const int INNER_RADIUS = OUTER_RADIUS - ARC_WIDTH;

void draw_thick_arc_on_canvas(lv_layer_t *layer,
                              int start_angle, int end_angle, 
                              lv_color_t color) {
    lv_draw_arc_dsc_t arc_dsc;
    lv_draw_arc_dsc_init(&arc_dsc);
    arc_dsc.color = color;
    arc_dsc.radius = 90;
    arc_dsc.width = 30;
    arc_dsc.rounded = 1;
    arc_dsc.center = {HALF_WIDTH, HALF_HEIGHT};
    arc_dsc.start_angle = start_angle;
    arc_dsc.end_angle = end_angle;
    lv_draw_arc(layer, &arc_dsc);

}

/**
 * @brief 最大３文字なので4バイト必要
 * 
 */
char battery_percent_buffer[4];

void draw_battery_clock() {
    lv_canvas_fill_bg(canvas, bg_color, LV_OPA_COVER);
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);
    int battery_percent = instance.pmu.getBatteryPercent();
    Serial.printf("battery percent: %d\n", battery_percent);
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = lv_color_black();
    label_dsc.font = &lv_font_montserrat_30;
    label_dsc.opa = LV_OPA_COVER;
    label_dsc.align = LV_TEXT_ALIGN_CENTER;
    // 微調整
    label_dsc.ofs_x = 1;
    label_dsc.ofs_y = 3;
    itoa(battery_percent, battery_percent_buffer, 10);
    label_dsc.text = battery_percent_buffer;
    lv_area_t area = {
        HALF_WIDTH - 30,
        HALF_HEIGHT - 20,
        HALF_WIDTH + 30,
        HALF_HEIGHT + 40,
    };
    label_dsc.align = LV_TEXT_ALIGN_CENTER;
    lv_draw_label(&layer, &label_dsc, &area);

    draw_thick_arc_on_canvas(&layer,
                             0, 360, lv_color_hex(0x303030));
    int indicator_end = (360 * battery_percent / 100);
    int isCharging = instance.pmu.isCharging();
    lv_color_t color = isCharging ? lv_color_hex(0x4CAF50) : (battery_percent <= 20 ? lv_color_hex(0xAF4C50) : lv_color_hex(0x504CAF));
    draw_thick_arc_on_canvas(&layer, 0, indicator_end, color);
    lv_canvas_finish_layer(canvas, &layer);
}

void battery_clock_gesture_callback() {
    // nothing to do
}
