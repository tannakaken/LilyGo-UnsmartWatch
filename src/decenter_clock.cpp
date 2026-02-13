#include <LilyGoLib.h>
#include <LV_Helper.h>
#include <WiFi.h>
#include <time.h>
#include <stdlib.h>
#include <esp_sntp.h>
#include "common.hpp"
#include "decenter_clock.hpp"


extern lv_obj_t *canvas;
extern lv_color_t bg_color;
extern struct tm timeinfo;
uint8_t hour_num = 12;

// 文字盤の文字のためのバッファ。バッファを使いまわすと上書きされてしまうため。
static char dial_buf[256];

/**
 * @brief 文字盤の半径
 * 
 */
static const float DIAL_RADIUS = 80.0f;

void calc_clock_coord(const float theta, lv_point_precise_t *point) {
    point->x = HALF_WIDTH + cos(theta - M_PI_2) * DIAL_RADIUS;
    point->y = HALF_HEIGHT + sin(theta - M_PI_2) * DIAL_RADIUS;
}

void drawline(lv_layer_t *layer, float theta, lv_color_t color, const float ratio, const uint8_t width) {
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = color;
    line_dsc.p1 = {static_cast<float>(center_x), static_cast<float>(center_y)};
    lv_point_precise_t target;
    calc_clock_coord(theta, &target);
    line_dsc.p2 = {ratio * (target.x - center_x) + center_x, ratio * (target.y - center_y) + center_y};
    line_dsc.width = width;
    lv_draw_line(layer, &line_dsc);
}


void draw_decenter_clock() {
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);
    lv_canvas_fill_bg(canvas, bg_color, LV_OPA_COVER);
    // 文字盤を描写
    for (uint8_t i = 1; i <= hour_num; ++i) {
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = lv_color_black();
        label_dsc.font = &lv_font_montserrat_10;
        label_dsc.opa = LV_OPA_COVER;
        label_dsc.align = LV_TEXT_ALIGN_CENTER;
        // 微調整
        label_dsc.ofs_x = 1;
        label_dsc.ofs_y = 3;
        // 文字盤用のbufferを4バイトずつ使う
        itoa(i, dial_buf + (4 * i), 10);
        label_dsc.text = dial_buf + (4 * i);
        float theta = static_cast<float>(i) * M_PI * 2 / hour_num;
        lv_point_precise_t target;
        calc_clock_coord(theta, &target);
        lv_area_t area = {
            static_cast<int32_t>(target.x) - 10,
            static_cast<int32_t>(target.y) - 10,
            static_cast<int32_t>(target.x) + 10,
            static_cast<int32_t>(target.y) + 10,
        };
        lv_draw_label(&layer, &label_dsc, &area);
    }

    drawline(
        &layer,
        timeinfo.tm_sec / 30.0 * M_PI,
        lv_palette_main(LV_PALETTE_RED),
        0.9,
        1
    );
    drawline(
        &layer,
        timeinfo.tm_min / 30.0 * M_PI + timeinfo.tm_sec / 1800.0f * M_PI,
        lv_color_black(),
        0.7,
        2
    );
    drawline(
        &layer,
        timeinfo.tm_hour / 6.0 * M_PI + timeinfo.tm_min / 360.0f * M_PI + timeinfo.tm_sec / 21600.0f * M_PI,
        lv_color_black(),
        0.4,
        3
    );

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);

    rect_dsc.bg_color = lv_color_black();
    rect_dsc.bg_opa   = LV_OPA_COVER;
    rect_dsc.border_width = 0;

    int radius = 4;
    int diameter = radius * 2;

    /* 角丸を最大にすることで円になる */
    rect_dsc.radius = radius;
    lv_area_t area = {
        center_x - radius,
        center_y - radius,
        center_x + radius,
        center_y + radius,
    };
    /* 円を描画 */
    lv_draw_rect(
        &layer,
        &rect_dsc,
        &area
    );


    lv_canvas_finish_layer(canvas, &layer);
}

void decenter_clock_gesture_callback()
{
    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) {
        return;
    }
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    int32_t diff_x = center_x - point.x;
    int32_t diff_y = center_y - point.y;
    float diff = sqrt(diff_x * diff_x + diff_y * diff_y);
    if (diff < 50) {
        center_x = point.x - 10;
        center_y = point.y - 10;
        draw_decenter_clock();
    }
}
