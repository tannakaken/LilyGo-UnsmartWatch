
/**
 * @file      main.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2025  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2025-05-15
 * 
 * Modifications:
 *   Copyright (c) 2026 Tannakaken
 *   Modified for custom smartwatch device project.
 * 
 * Based on original work by https://github.com/Xinyuan-LilyGO/LilyGoLib-PlatformIO
 */
#include <LilyGoLib.h>
#include <LV_Helper.h>
#include <WiFi.h>
#include <time.h>
#include <stdlib.h>
#include <esp_sntp.h>
#include "common.hpp"
#include "decenter_clock.hpp"
#include "battery_clock.hpp"


lv_obj_t *mbox;
#define SSID_LENGTH 32
char selected_ssid[SSID_LENGTH + 1];
const char CHARS[] = "abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNQRTUVWXYZ23456789";
#define PASSWORD_LENGTH 13
char random_password[PASSWORD_LENGTH + 1];


const char *ntpServer1 = "ntp.jst.mfeed.ad.jp";
const char *ntpServer2 = "time.cloudflare.com";

// UTC+9な気がするけど、こうするとうまくいく。JTCでもうまくいかない。
const char *time_zone = "UTC-9";
bool updated = false;

void timeavailable(struct timeval *t)
{
    Serial.println("Got time adjustment from NTP, Write the hardware clock");

    // 合わせた時計をハードウェアクロックに書き込む。
    instance.rtc.hwClockWrite();

    WiFi.disconnect();
    updated = true;
    // ここでメッセージボックスを出すと
    // Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled.
    // が出て落ちる。
}


static void connect_event_callback(lv_event_t *event) {
    Serial.printf("SSID:%s\n", selected_ssid);
    Serial.printf("Password:%s\n", random_password);
    WiFi.begin(selected_ssid, random_password);
    uint8_t counter = 0;
    const uint8_t LIMIT = 20;
    while (!WiFi.isConnected()) {
        delay(500);
        Serial.print(".");
        ++counter;
        if (counter == LIMIT) {
            Serial.println("Can not connect WIFI");
            WiFi.disconnect();
            break;
        }
    }
    if (mbox) {
        lv_obj_t * gray_bg = lv_obj_get_parent(mbox);
        lv_obj_del(gray_bg);
    }
    if (counter == LIMIT) {
        lv_obj_t *alert_box = lv_msgbox_create(NULL);
        lv_obj_set_size(alert_box, LV_PCT(80), LV_SIZE_CONTENT);
        lv_msgbox_add_title(alert_box, "Can not connect Wifi");
        lv_msgbox_add_close_button(alert_box);
    }
}

static void generate_random_password() {
    // ヌル終端文字の分を除く
    const size_t SIZE_OF_CHARS = sizeof(CHARS) / sizeof(CHARS[0]) - 1;
    for (uint8_t i = 0; i < PASSWORD_LENGTH; ++i) {
        size_t index = rand() % SIZE_OF_CHARS;
        random_password[i] = CHARS[index];
    }
}


static void select_ssid_event_callback(lv_event_t *event) {
    lv_obj_t *clicked_button = lv_event_get_target_obj(event);
    // 一つ目はアイコンなので
    lv_obj_t *label = lv_obj_get_child(clicked_button, 1);
    strncpy(selected_ssid, lv_label_get_text(label), SSID_LENGTH);
    Serial.println(selected_ssid);
    if (mbox) {
        lv_obj_t * gray_bg = lv_obj_get_parent(mbox);
        lv_obj_del(gray_bg);
    }
    mbox = lv_msgbox_create(NULL);
    lv_obj_set_size(mbox, LV_PCT(80), LV_PCT(80));
    lv_msgbox_add_title(mbox, "Connect Wifi");
    lv_msgbox_add_close_button(mbox);

    lv_obj_t *ssid_text = lv_label_create(mbox);
    lv_label_set_text(ssid_text, selected_ssid);
    lv_obj_align_to(ssid_text, mbox, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *password_text = lv_label_create(mbox);
    generate_random_password();
    lv_label_set_text(password_text, random_password);
    lv_obj_align_to(password_text, ssid_text, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    lv_obj_t *text = lv_label_create(mbox);
    lv_label_set_text(text, "Please change password and click button");
    lv_obj_align_to(text, password_text, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_obj_t *button = lv_button_create(mbox);
    lv_obj_align_to(button, text, LV_ALIGN_OUT_BOTTOM_MID, 0, 00);
    lv_obj_t *button_label = lv_label_create(button);
    lv_label_set_text(button_label, "Connect!");
    lv_obj_add_event_cb(button, connect_event_callback, LV_EVENT_CLICKED, NULL);
    lv_obj_center(button_label);
}

/**
 * @brief Set the time event callback object
 * 
 * Lilygo T-Watch S3ではクラウン（長押しするとスリープ、スリープ状態で押すと復帰）の
 * クリックイベントをうまく拾えなかったので画面全体の長押しで時計合わせにした。
 * 
 * @param event 
 */
static void set_time_event_callback(lv_event_t *event)
{
    mbox = lv_msgbox_create(NULL);
    lv_obj_set_size(mbox, LV_PCT(80), LV_PCT(80));
    lv_msgbox_add_title(mbox, "WiFi List");
    lv_msgbox_add_close_button(mbox);

    lv_obj_t *text = lv_label_create(mbox);
    lv_label_set_text(text, "Choose a Wi-Fi with a changeable password.");
    lv_obj_set_size(text, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align_to(text, mbox, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *list = lv_list_create(mbox);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(text, LV_PCT(100), LV_SIZE_CONTENT);
    int16_t result = WiFi.scanNetworks();
    if (result == WIFI_SCAN_FAILED) {
        Serial.println("WiFi Scan Failed");
    } else {
        uint8_t ssid_num = (uint8_t)result;
        for (uint8_t i = 0; i < ssid_num; ++i) {
            String ssid = WiFi.SSID(i);
            lv_obj_t *button = lv_list_add_btn(list, LV_SYMBOL_WIFI, ssid.c_str());
            lv_obj_add_event_cb(button, select_ssid_event_callback, LV_EVENT_CLICKED, NULL);
        }
    }
    lv_obj_align_to(list, text, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
}

lv_obj_t *canvas;
lv_color_t bg_color;
struct tm timeinfo;

enum ClockMode {
    DECENTER_CLOCK_MODE = 0,
    BATTERY_CLOCK_MODE = 1,
};

static ClockMode clock_mode = DECENTER_CLOCK_MODE;

void draw_clock() {
    switch (clock_mode) {
    case DECENTER_CLOCK_MODE:
        draw_decenter_clock();
        break;
    case BATTERY_CLOCK_MODE:
        draw_battery_clock();
        break;

    }
}

void gesture_callback(lv_event_t * event)
{
    switch (clock_mode)
    {
    case DECENTER_CLOCK_MODE:
        decenter_clock_gesture_callback();
        break;
    case BATTERY_CLOCK_MODE:
        battery_clock_gesture_callback();
        break;
    } 
}


/**
 * @brief 現在は電源ボタンのクリックのみ処理している。
 * 
 * 詳しくは.pio/libdeps/LilyGolib/exanokes/power/PowerManageEvent/PowerManageEvent.inoを参照せよ。
 * 
 * @param event 
 * @param params 
 * @param user_data 
 */
void power_event_cb(DeviceEvent_t event, void*params, void*user_data)
{
    if (event != POWER_EVENT) {
        return;
    }
    switch (instance.getPMUEventType(params)) {
    case PMU_EVENT_KEY_CLICKED:
        Serial.println("Power button is clicked");
        if (clock_mode == BATTERY_CLOCK_MODE) {
            clock_mode = DECENTER_CLOCK_MODE;
        } else {
            clock_mode = static_cast<ClockMode>(clock_mode + 1);
        }
        break;
    default:
        break;
    }
}

uint32_t last_milliseconds;
char buf[64];
void setup()
{
    // 乱数の初期化
    srand((unsigned)time(NULL));
    // ランダムパスワードのヌル文字の設定
    random_password[PASSWORD_LENGTH] = '\0';
    selected_ssid[SSID_LENGTH] = '\0';

    // 時計合わせ。これらの関数はWiFiにつながる前に呼べばよい。
    sntp_set_time_sync_notification_cb(timeavailable);

    /**
     * NTP server address could be aquired via DHCP,
     *
     * NOTE: This call should be made BEFORE esp32 aquires IP address via DHCP,
     * otherwise SNTP option 42 would be rejected by default.
     * NOTE: configTime() function call if made AFTER DHCP-client run
     * will OVERRIDE aquired NTP server address
     */
    sntp_servermode_dhcp(1);    // (optional)

    configTzTime(time_zone, ntpServer1, ntpServer2);
    
    Serial.begin(115200);

    instance.begin();

    WiFi.mode(WIFI_STA);

    beginLvglHelper(instance);

    lv_obj_t *screen = lv_screen_active();

    lv_obj_add_event_cb(screen, set_time_event_callback, LV_EVENT_DOUBLE_CLICKED, NULL);
    // canvasにコールバックを追加してもなぜか動かなかった。
    lv_obj_add_event_cb(screen, gesture_callback, LV_EVENT_PRESSING, NULL);

    LV_DRAW_BUF_DEFINE_STATIC(draw_buf_32bpp, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_RGB888);
    LV_DRAW_BUF_INIT_STATIC(draw_buf_32bpp);

    canvas = lv_canvas_create(screen);
    lv_canvas_set_draw_buf(canvas, &draw_buf_32bpp);
    lv_obj_set_size(canvas, CANVAS_WIDTH, CANVAS_HEIGHT);
    lv_obj_center(canvas);
    bg_color = lv_color_make(200, 200, 200);
    lv_canvas_fill_bg(canvas, bg_color, LV_OPA_COVER);

    // Clear all interrupt status
    instance.pmu.clearIrqStatus();

    // Enable the required interrupt function
    // 電源管理系のイベントを処理するためには、これをenableする必要がある。
    // 電源ボタンのクリックのみ登録。
    // 電源ボタンの長押しや、vbus(USB電源)の接続・切断、
    // バッテリーの接続切断、重電の開始・終了などのイベントがenableできる。
    // 詳しくは.pio/libdeps/LilyGolib/exanokes/power/PowerManageEvent/PowerManageEvent.inoを参照せよ。
    instance.pmu.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ);

    // バッテリーの充電率を確認できるようにする。
    // 詳しくは.pio/libdeps/LilyGolib/exanokes/power/PowerManageMonitor/PowerManageMonitor.inoを参照せよ。
    instance.pmu.enableBattDetection();

    // Set brightness to MAX
    // T-LoRa-Pager brightness level is 0 ~ 16
    // T-Watch-S3 , T-Watch-S3-Plus , T-Watch-Ultra brightness level is 0 ~ 255
    instance.setBrightness(DEVICE_MAX_BRIGHTNESS_LEVEL);

    // Register power event
    instance.onEvent(power_event_cb, POWER_EVENT, NULL);
}


void loop()
{
    if (updated) {
        lv_obj_t *success_box = lv_msgbox_create(NULL);
        lv_obj_set_size(success_box, LV_PCT(80), LV_SIZE_CONTENT);
        lv_msgbox_add_title(success_box, "The time has been adjusted!");
        lv_msgbox_add_close_button(success_box);
        updated = false;
    }

    uint32_t now_milliseconds = millis();
    if (now_milliseconds - last_milliseconds > 1000) {

        last_milliseconds = now_milliseconds;

        
        // Get the time C library structure
        instance.rtc.getDateTime(&timeinfo);

        draw_clock();

        // Format the output using the strftime function
        // For more formats, please refer to :
        // https://man7.org/linux/man-pages/man3/strftime.3.html

        size_t written = strftime(buf, 64, "%Y-%m-%d(%a) %H:%M:%S", &timeinfo);

        if (written != 0) {
            Serial.println(buf);
        }
    }
    // instance.onEventを処理するためにはこれを呼びだす必要がある。
    instance.loop();
    lv_task_handler();
    delay(5);
}
