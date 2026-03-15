/**
 * ui.c  –  LVGL 8.3 Demo UI  (v7)
 * Screen : 160 × 128
 *
 * Changes v7:
 *  - APP buttons wider (smaller pad/gap)
 *  - Deeper button & icon-circle colours
 *  - Icon↔text gap added in info rows (INFO_X_TEXT shifted right)
 *  - Battery changed to percentage (default 80%)
 *
 * lv_conf.h fonts required:
 *   #define LV_FONT_MONTSERRAT_12  1
 *   #define LV_FONT_MONTSERRAT_14  1
 *   #define LV_FONT_MONTSERRAT_28  1
 */

#include "ui.h"
#include "ui_event.h"
#include "lvgl/lvgl.h"
#include <pthread.h>
#include <unistd.h>
#include "lvgl/src/extra/libs/qrcode/lv_qrcode.h"
#include "devices/threads/threads_conf.h"

extern lv_font_t ui_font_AlimamaShuHeiTi;

/* ═══════════════════════════════════════
 *  Colour Palette  (v7: deeper tones)
 * ═══════════════════════════════════════ */
#define C_BG            lv_color_hex(0xFFF0F0)   /* screen background            */
#define C_ACCENT        lv_color_hex(0xCC0000)   /* red: time, icons, titles     */
#define C_TEXT          lv_color_hex(0x1A1A1A)   /* near-black body text         */
#define C_DIVIDER       lv_color_hex(0xDDB0B0)   /* soft vertical separator      */

/* ── APP button colours (deeper than before) ── */
#define C_BTN           lv_color_hex(0xF4AAAA)   /* button face  (was FFD6D6)    */
#define C_BTN_PRESS     lv_color_hex(0xE07070)   /* button pressed (was FFAAAA)  */
#define C_ICON_CIRCLE   lv_color_hex(0xDD7070)   /* icon circle  (was FF9999)    */

/* ── Back-button on sub-screens (deeper) ── */
#define C_BACK_BTN      lv_color_hex(0xE08080)   /* face  (was FFD6D6)           */
#define C_BACK_BTN_PR   lv_color_hex(0xC05555)   /* pressed                      */

/* ═══════════════════════════════════════
 *  Screen & top-panel constants
 * ═══════════════════════════════════════ */
#define SCR_W             160
#define SCR_H             128
#define TOP_H              64
#define BOT_H              64

/* Time label */
#define DIV_X              82          /* divider x  (fits "23:28" @ size-28) */
#define TIME_LABEL_W       (DIV_X - 6) /* = 76px, single line                 */
#define TIME_LABEL_X        3

/* Right info panel
 *   Icon at INFO_X_ICON, text at INFO_X_TEXT.
 *   Gap between icon and text = INFO_X_TEXT - INFO_X_ICON - ~14(icon w) ≈ 6px
 */
#define INFO_X_ICON        (DIV_X + 4)  /* = 86  */
#define INFO_X_TEXT        (DIV_X + 24) /* = 106  ← +4px gap vs. v6 (was +20) */
#define INFO_TEXT_W        (SCR_W - INFO_X_TEXT - 2) /* = 52px, fits IPs      */
#define INFO_ROW_H          18
#define INFO_Y_START         5

/* ═══════════════════════════════════════
 *  Bottom-bar constants  (v7: wider buttons)
 *
 *  BTN_PAD_OUTER : 3px (was 5) – outer left/right margin of container
 *  BTN_GAP       : 3px (was 5) – gap between buttons
 *  BTN_W = (160 - 3*2 - 3*2) / 3 = 148/3 = 49px  (was 46px)
 *  BTN_CIRCLE_D  : 30px (was 28)
 * ═══════════════════════════════════════ */
#define BTN_PAD_OUTER       3
#define BTN_GAP             3
#define BTN_COUNT           3
#define BTN_W  ((SCR_W - BTN_PAD_OUTER * 2 - BTN_GAP * (BTN_COUNT - 1)) / BTN_COUNT)
                                         /* = (160-6-6)/3 = 49px               */
#define BTN_H               (BOT_H - 12) /* = 52px                             */
#define BTN_RADIUS           8
#define BTN_CIRCLE_D         30          /* icon circle diameter               */

/* ═══════════════════════════════════════
 *  Global label handles
 * ═══════════════════════════════════════ */
static lv_obj_t *g_lbl_time;
static lv_obj_t *g_lbl_cpu;
static lv_obj_t *g_lbl_bat;
static lv_obj_t *g_lbl_ip;
static lv_obj_t *g_power_mask;
static lv_obj_t *g_power_dialog;
static lv_obj_t *g_wifi_roller;
static lv_obj_t *g_wifi_title;
static lv_obj_t *g_wifi_mask;
static lv_obj_t *g_wifi_qr_dialog;
static lv_obj_t *g_wifi_qr_ssid_label;
static lv_obj_t *g_wifi_qr_obj;
static lv_obj_t *g_wifi_qr_hint_label;
static lv_obj_t *g_wifi_loading_mask;
static lv_obj_t *g_wifi_loading_dialog;
static lv_timer_t *g_wifi_ui_dispatch_timer;
static char g_wifi_selected_ssid[64];
static char g_wifi_submitted_password[64];
static char g_wifi_portal_hint[160];
static char g_wifi_roller_options[512] = "option1\noption2\noption3";
static pthread_mutex_t g_wifi_ui_request_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_wifi_pending_qr_update = false;
static bool g_wifi_pending_loading_close = false;
static bool g_wifi_pending_title_update = false;
static bool g_wifi_pending_scan_results_update = false;
static bool g_wifi_pending_submit_password = false;
static char g_wifi_pending_qr_payload[256];
static char g_wifi_pending_qr_hint[160];
static char g_wifi_pending_title[128];
static char g_wifi_pending_scan_results[512];
static char g_wifi_pending_password[64];

/* ═══════════════════════════════════════
 *  Power dialog constants
 * ═══════════════════════════════════════ */
#define PWR_DLG_W          148
#define PWR_DLG_H           60
#define PWR_DLG_BTN_W       66
#define PWR_DLG_BTN_H       26
#define C_MODAL_MASK      lv_color_hex(0x000000)
#define C_MODAL_PANEL     lv_color_hex(0xFFF8F8)
#define C_MODAL_BORDER    lv_color_hex(0xD88A8A)

/* ═══════════════════════════════════════
 *  Wi-Fi UI constants
 * ═══════════════════════════════════════ */
#define WIFI_TITLE_Y         6
#define WIFI_TITLE_W       115
#define WIFI_ROLLER_X       10
#define WIFI_ROLLER_Y       25
#define WIFI_ROLLER_W      140
#define WIFI_ROLLER_H       42
#define WIFI_BTN_W          60
#define WIFI_BTN_H          25
#define WIFI_BTN_Y          95

#define WIFI_QR_DLG_W      148
#define WIFI_QR_DLG_H      124
#define WIFI_QR_SIZE        52
#define WIFI_QR_BTN_W       65
#define WIFI_QR_BTN_H       20

#define WIFI_LOADING_W     118
#define WIFI_LOADING_H      58

static void wifi_modal_delete_cb(lv_event_t *e)
{
    (void)e;
    g_wifi_mask = NULL;
    g_wifi_qr_dialog = NULL;
    g_wifi_qr_ssid_label = NULL;
    g_wifi_qr_obj = NULL;
    g_wifi_qr_hint_label = NULL;
}

static void wifi_loading_delete_cb(lv_event_t *e)
{
    (void)e;
    g_wifi_loading_mask = NULL;
    g_wifi_loading_dialog = NULL;
}

static void power_dialog_delete_cb(lv_event_t *e)
{
    (void)e;
    g_power_mask = NULL;
    g_power_dialog = NULL;
}

static lv_obj_t *build_power_action_btn(lv_obj_t *parent,
                                        const char *text,
                                        lv_coord_t x,
                                        lv_event_cb_t event_cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, PWR_DLG_BTN_W, PWR_DLG_BTN_H);
    lv_obj_set_pos(btn, x, 28);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_bg_color(btn, C_BTN, 0);
    lv_obj_set_style_bg_color(btn, C_BTN_PRESS, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label, C_TEXT, 0);
    lv_obj_center(label);

    return btn;
}

static lv_obj_t *build_modal_action_btn(lv_obj_t *parent,
                                        const char *text,
                                        lv_coord_t x,
                                        lv_coord_t y,
                                        lv_coord_t width,
                                        lv_coord_t height,
                                        lv_event_cb_t event_cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, width, height);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_bg_color(btn, C_BTN, 0);
    lv_obj_set_style_bg_color(btn, C_BTN_PRESS, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label, C_TEXT, 0);
    lv_obj_center(label);

    return btn;
}

static void wifi_get_selected_ssid(char *buffer, size_t buffer_size)
{
    if(!g_wifi_roller || !lv_obj_is_valid(g_wifi_roller))
    {
        snprintf(buffer, buffer_size, "option1");
        return;
    }

    lv_roller_get_selected_str(g_wifi_roller, buffer, buffer_size);
}

static void wifi_apply_roller_options(void)
{
    if(!g_wifi_roller || !lv_obj_is_valid(g_wifi_roller))
    {
        return;
    }

    lv_roller_set_options(g_wifi_roller, g_wifi_roller_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(g_wifi_roller, 0, LV_ANIM_OFF);
    g_wifi_selected_ssid[0] = '\0';
}

static void wifi_ui_dispatch_timer_cb(lv_timer_t *timer)
{
    bool has_qr_update;
    bool has_loading_close;
    bool has_title_update;
    bool has_scan_results_update;
    bool has_submit_password;
    char qr_payload[256];
    char qr_hint[160];
    char title[128];
    char scan_results[512];
    char password[64];

    (void)timer;

    qr_payload[0] = '\0';
    qr_hint[0] = '\0';
    title[0] = '\0';
    scan_results[0] = '\0';
    password[0] = '\0';

    pthread_mutex_lock(&g_wifi_ui_request_mutex);
    has_qr_update = g_wifi_pending_qr_update;
    has_loading_close = g_wifi_pending_loading_close;
    has_title_update = g_wifi_pending_title_update;
    has_scan_results_update = g_wifi_pending_scan_results_update;
    has_submit_password = g_wifi_pending_submit_password;

    if(has_qr_update)
    {
        snprintf(qr_payload, sizeof(qr_payload), "%s", g_wifi_pending_qr_payload);
        snprintf(qr_hint, sizeof(qr_hint), "%s", g_wifi_pending_qr_hint);
        g_wifi_pending_qr_update = false;
    }

    if(has_loading_close)
    {
        g_wifi_pending_loading_close = false;
    }

    if(has_title_update)
    {
        snprintf(title, sizeof(title), "%s", g_wifi_pending_title);
        g_wifi_pending_title_update = false;
    }

    if(has_scan_results_update)
    {
        snprintf(scan_results, sizeof(scan_results), "%s", g_wifi_pending_scan_results);
        g_wifi_pending_scan_results_update = false;
    }

    if(has_submit_password)
    {
        snprintf(password, sizeof(password), "%s", g_wifi_pending_password);
        g_wifi_pending_submit_password = false;
    }
    pthread_mutex_unlock(&g_wifi_ui_request_mutex);

    if(has_title_update)
    {
        ui_wifi_set_title(title);
    }

    if(has_scan_results_update)
    {
        ui_wifi_set_scan_results(scan_results);
    }

    if(has_qr_update)
    {
        ui_wifi_qr_dialog_update(qr_payload, qr_hint);
    }

    if(has_submit_password)
    {
        ui_wifi_submit_phone_password(password);
    }

    if(has_loading_close)
    {
        ui_wifi_loading_close();
    }
}

/* ════════════════════════════════════════════════════════
 *  build_back_button  (deeper colour scheme)
 * ════════════════════════════════════════════════════════ */
static void build_back_button(lv_obj_t *parent)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 28, 22);
    lv_obj_set_pos(btn, 2, 2);
    lv_obj_set_style_radius(btn, 4, 0);
    lv_obj_set_style_bg_color(btn, C_BACK_BTN, 0);
    lv_obj_set_style_bg_color(btn, C_BACK_BTN_PR, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, ui_event_back_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0); /* white arrow */
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);
}

/* ════════════════════════════════════════════════════════
 *  ui_camera_screen_create
 * ════════════════════════════════════════════════════════ */
lv_obj_t *ui_camera_screen_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, C_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    build_back_button(scr);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Camera");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, C_ACCENT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* ── user adds widgets here ── */
    return scr;
}

/* ════════════════════════════════════════════════════════
 *  ui_wifi_screen_create
 * ════════════════════════════════════════════════════════ */
lv_obj_t *ui_wifi_screen_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    char wifi_title_text[128];
    lv_obj_set_style_bg_color(scr, C_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    build_back_button(scr);

    g_wifi_title = lv_label_create(scr);
    wifi_build_status_text(wifi_title_text, sizeof(wifi_title_text));
    lv_label_set_text(g_wifi_title, wifi_title_text);
    lv_obj_set_width(g_wifi_title, WIFI_TITLE_W);
    lv_label_set_long_mode(g_wifi_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(g_wifi_title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(g_wifi_title, C_ACCENT, 0);
    lv_obj_set_style_text_align(g_wifi_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(g_wifi_title, LV_ALIGN_TOP_MID, 8, WIFI_TITLE_Y + 1);

    g_wifi_roller = lv_roller_create(scr);
    lv_obj_set_size(g_wifi_roller, WIFI_ROLLER_W, WIFI_ROLLER_H);
    lv_obj_set_pos(g_wifi_roller, WIFI_ROLLER_X, WIFI_ROLLER_Y);
    lv_roller_set_options(g_wifi_roller, g_wifi_roller_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_wifi_roller, 3);
    lv_obj_set_style_radius(g_wifi_roller, 8, 0);
    lv_obj_set_style_bg_color(g_wifi_roller, C_MODAL_PANEL, 0);
    lv_obj_set_style_bg_opa(g_wifi_roller, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(g_wifi_roller, C_MODAL_BORDER, 0);
    lv_obj_set_style_border_width(g_wifi_roller, 1, 0);
    lv_obj_set_style_text_color(g_wifi_roller, C_TEXT, 0);
    lv_obj_set_style_text_font(g_wifi_roller, &ui_font_AlimamaShuHeiTi, 0);
    lv_obj_set_style_text_color(g_wifi_roller, C_ACCENT, LV_PART_SELECTED);
    lv_obj_set_style_text_font(g_wifi_roller, &ui_font_AlimamaShuHeiTi, LV_PART_SELECTED);
    lv_obj_set_style_bg_color(g_wifi_roller, lv_color_hex(0xFFD6D6), LV_PART_SELECTED);

    build_modal_action_btn(scr,
                           "Fresh",
                           14,
                           WIFI_BTN_Y,
                           WIFI_BTN_W,
                           WIFI_BTN_H,
                           ui_event_wifi_refresh_btn);
    build_modal_action_btn(scr,
                           "Connect",
                           86,
                           WIFI_BTN_Y,
                           WIFI_BTN_W,
                           WIFI_BTN_H,
                           ui_event_wifi_connect_btn);

    return scr;
}

void ui_wifi_set_title(const char *text)
{
    if(!g_wifi_title || !lv_obj_is_valid(g_wifi_title))
    {
        return;
    }

    lv_label_set_text(g_wifi_title, (text && text[0] != '\0') ? text : "Disconnecting");
}

void ui_wifi_set_scan_results(const char *options)
{
    if(options && options[0] != '\0')
    {
        snprintf(g_wifi_roller_options, sizeof(g_wifi_roller_options), "%s", options);
    }
    else
    {
        snprintf(g_wifi_roller_options, sizeof(g_wifi_roller_options), "No Wi-Fi Found");
    }

    wifi_apply_roller_options();
}

void ui_wifi_qr_dialog_show(void)
{
    lv_obj_t *scr = lv_scr_act();
    char selected_ssid[64];

    if(g_wifi_mask && lv_obj_is_valid(g_wifi_mask) && lv_obj_get_parent(g_wifi_mask) == scr)
    {
        lv_obj_move_foreground(g_wifi_mask);
        return;
    }

    wifi_get_selected_ssid(selected_ssid, sizeof(selected_ssid));
    snprintf(g_wifi_selected_ssid, sizeof(g_wifi_selected_ssid), "%s", selected_ssid);
    snprintf(g_wifi_portal_hint, sizeof(g_wifi_portal_hint), "%s", "Preparing portal...");

    g_wifi_mask = lv_obj_create(scr);
    lv_obj_set_size(g_wifi_mask, SCR_W, SCR_H);
    lv_obj_set_pos(g_wifi_mask, 0, 0);
    lv_obj_set_style_bg_color(g_wifi_mask, C_MODAL_MASK, 0);
    lv_obj_set_style_bg_opa(g_wifi_mask, LV_OPA_40, 0);
    lv_obj_set_style_border_width(g_wifi_mask, 0, 0);
    lv_obj_set_style_radius(g_wifi_mask, 0, 0);
    lv_obj_set_style_pad_all(g_wifi_mask, 0, 0);
    lv_obj_clear_flag(g_wifi_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(g_wifi_mask, wifi_modal_delete_cb, LV_EVENT_DELETE, NULL);

    g_wifi_qr_dialog = lv_obj_create(g_wifi_mask);
    lv_obj_set_size(g_wifi_qr_dialog, WIFI_QR_DLG_W, WIFI_QR_DLG_H);
    lv_obj_center(g_wifi_qr_dialog);
    lv_obj_set_style_bg_color(g_wifi_qr_dialog, C_MODAL_PANEL, 0);
    lv_obj_set_style_bg_opa(g_wifi_qr_dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_wifi_qr_dialog, 1, 0);
    lv_obj_set_style_border_color(g_wifi_qr_dialog, C_MODAL_BORDER, 0);
    lv_obj_set_style_radius(g_wifi_qr_dialog, 10, 0);
    lv_obj_set_style_shadow_width(g_wifi_qr_dialog, 8, 0);
    lv_obj_set_style_shadow_color(g_wifi_qr_dialog, lv_color_hex(0xAA7070), 0);
    lv_obj_set_style_shadow_opa(g_wifi_qr_dialog, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(g_wifi_qr_dialog, 0, 0);
    lv_obj_clear_flag(g_wifi_qr_dialog, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(g_wifi_qr_dialog);
    lv_label_set_text(title, "Scan On Phone");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(title, C_ACCENT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    g_wifi_qr_ssid_label = lv_label_create(g_wifi_qr_dialog);
    lv_label_set_text(g_wifi_qr_ssid_label, selected_ssid);
    lv_obj_set_width(g_wifi_qr_ssid_label, WIFI_QR_DLG_W - 16);
    lv_label_set_long_mode(g_wifi_qr_ssid_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(g_wifi_qr_ssid_label, &ui_font_AlimamaShuHeiTi, 0);
    lv_obj_set_style_text_color(g_wifi_qr_ssid_label, C_TEXT, 0);
    lv_obj_set_style_text_align(g_wifi_qr_ssid_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(g_wifi_qr_ssid_label, LV_ALIGN_TOP_MID, 0, 18);

    g_wifi_qr_obj = lv_qrcode_create(g_wifi_qr_dialog,
                                     WIFI_QR_SIZE,
                                     lv_color_hex(0x1A1A1A),
                                     lv_color_hex(0xFFFFFF));
    lv_qrcode_update(g_wifi_qr_obj, "Preparing", strlen("Preparing"));
    lv_obj_align(g_wifi_qr_obj, LV_ALIGN_TOP_MID, 0, 32);

    g_wifi_qr_hint_label = lv_label_create(g_wifi_qr_dialog);
    lv_label_set_text(g_wifi_qr_hint_label,
                      g_wifi_portal_hint[0] != '\0' ? g_wifi_portal_hint : "Open phone portal");
    lv_obj_set_width(g_wifi_qr_hint_label, WIFI_QR_DLG_W - 10);
    lv_label_set_long_mode(g_wifi_qr_hint_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(g_wifi_qr_hint_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(g_wifi_qr_hint_label, C_TEXT, 0);
    lv_obj_set_style_text_align(g_wifi_qr_hint_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(g_wifi_qr_hint_label, LV_ALIGN_TOP_MID, 0, 83);

    build_modal_action_btn(g_wifi_qr_dialog,
                           "Cancel",
                           (WIFI_QR_DLG_W - WIFI_QR_BTN_W) / 2,
                           99,
                           WIFI_QR_BTN_W,
                           WIFI_QR_BTN_H,
                           ui_event_wifi_qr_cancel_btn);
}

void ui_wifi_qr_dialog_update(const char *qr_payload, const char *hint)
{
    if(!g_wifi_qr_dialog || !lv_obj_is_valid(g_wifi_qr_dialog) ||
       !g_wifi_qr_obj || !lv_obj_is_valid(g_wifi_qr_obj) ||
       !g_wifi_qr_hint_label || !lv_obj_is_valid(g_wifi_qr_hint_label))
    {
        return;
    }

    if(qr_payload && qr_payload[0] != '\0')
    {
        lv_qrcode_update(g_wifi_qr_obj, qr_payload, strlen(qr_payload));
        lv_obj_invalidate(g_wifi_qr_obj);
        printf("[wifi] qr payload: %s\n", qr_payload);
    }

    if(hint && hint[0] != '\0')
    {
        snprintf(g_wifi_portal_hint, sizeof(g_wifi_portal_hint), "%s", hint);
        lv_label_set_text(g_wifi_qr_hint_label, g_wifi_portal_hint);
    }
}

void ui_wifi_request_qr_dialog_update(const char *qr_payload, const char *hint)
{
    pthread_mutex_lock(&g_wifi_ui_request_mutex);
    snprintf(g_wifi_pending_qr_payload,
             sizeof(g_wifi_pending_qr_payload),
             "%s",
             qr_payload ? qr_payload : "");
    snprintf(g_wifi_pending_qr_hint,
             sizeof(g_wifi_pending_qr_hint),
             "%s",
             hint ? hint : "");
    g_wifi_pending_qr_update = true;
    pthread_mutex_unlock(&g_wifi_ui_request_mutex);
}

void ui_wifi_qr_dialog_close(void)
{
    if(g_wifi_mask && lv_obj_is_valid(g_wifi_mask))
    {
        lv_obj_del(g_wifi_mask);
    }
}

void ui_wifi_submit_phone_password(const char *password)
{
    snprintf(g_wifi_submitted_password,
             sizeof(g_wifi_submitted_password),
             "%s",
             password ? password : "");

    ui_wifi_qr_dialog_close();
    ui_wifi_loading_show();
}

const char *ui_wifi_get_selected_ssid(void)
{
    if(g_wifi_selected_ssid[0] == '\0')
    {
        wifi_get_selected_ssid(g_wifi_selected_ssid, sizeof(g_wifi_selected_ssid));
    }

    return g_wifi_selected_ssid;
}

const char *ui_wifi_get_submitted_password(void)
{
    return g_wifi_submitted_password;
}

void ui_wifi_loading_show(void)
{
    lv_obj_t *scr = lv_scr_act();
    char selected_ssid[64];

    if(g_wifi_loading_mask && lv_obj_is_valid(g_wifi_loading_mask) && lv_obj_get_parent(g_wifi_loading_mask) == scr)
    {
        lv_obj_move_foreground(g_wifi_loading_mask);
        return;
    }

    snprintf(selected_ssid,
             sizeof(selected_ssid),
             "%s",
             ui_wifi_get_selected_ssid());

    g_wifi_loading_mask = lv_obj_create(scr);
    lv_obj_set_size(g_wifi_loading_mask, SCR_W, SCR_H);
    lv_obj_set_pos(g_wifi_loading_mask, 0, 0);
    lv_obj_set_style_bg_color(g_wifi_loading_mask, C_MODAL_MASK, 0);
    lv_obj_set_style_bg_opa(g_wifi_loading_mask, LV_OPA_40, 0);
    lv_obj_set_style_border_width(g_wifi_loading_mask, 0, 0);
    lv_obj_set_style_radius(g_wifi_loading_mask, 0, 0);
    lv_obj_set_style_pad_all(g_wifi_loading_mask, 0, 0);
    lv_obj_clear_flag(g_wifi_loading_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(g_wifi_loading_mask, wifi_loading_delete_cb, LV_EVENT_DELETE, NULL);

    g_wifi_loading_dialog = lv_obj_create(g_wifi_loading_mask);
    lv_obj_set_size(g_wifi_loading_dialog, WIFI_LOADING_W, WIFI_LOADING_H);
    lv_obj_center(g_wifi_loading_dialog);
    lv_obj_set_style_bg_color(g_wifi_loading_dialog, C_MODAL_PANEL, 0);
    lv_obj_set_style_bg_opa(g_wifi_loading_dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_wifi_loading_dialog, 1, 0);
    lv_obj_set_style_border_color(g_wifi_loading_dialog, C_MODAL_BORDER, 0);
    lv_obj_set_style_radius(g_wifi_loading_dialog, 10, 0);
    lv_obj_set_style_shadow_width(g_wifi_loading_dialog, 8, 0);
    lv_obj_set_style_shadow_color(g_wifi_loading_dialog, lv_color_hex(0xAA7070), 0);
    lv_obj_set_style_shadow_opa(g_wifi_loading_dialog, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(g_wifi_loading_dialog, 0, 0);
    lv_obj_clear_flag(g_wifi_loading_dialog, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *spinner = lv_spinner_create(g_wifi_loading_dialog, 800, 60);
    lv_obj_set_size(spinner, 24, 24);
    lv_obj_align(spinner, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_arc_color(spinner, C_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 3, LV_PART_INDICATOR);

    lv_obj_t *title = lv_label_create(g_wifi_loading_dialog);
    lv_label_set_text(title, "Connected");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(title, C_ACCENT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 10, 8);

    lv_obj_t *ssid = lv_label_create(g_wifi_loading_dialog);
    lv_label_set_text(ssid, selected_ssid);
    lv_obj_set_width(ssid, 72);
    lv_label_set_long_mode(ssid, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(ssid, &ui_font_AlimamaShuHeiTi, 0);
    lv_obj_set_style_text_color(ssid, C_TEXT, 0);
    lv_obj_align(ssid, LV_ALIGN_RIGHT_MID, -8, 10);
}

void ui_wifi_loading_close(void)
{
    if(g_wifi_loading_mask && lv_obj_is_valid(g_wifi_loading_mask))
    {
        lv_obj_del(g_wifi_loading_mask);
    }
}

void ui_wifi_request_loading_close(void)
{
    pthread_mutex_lock(&g_wifi_ui_request_mutex);
    g_wifi_pending_loading_close = true;
    pthread_mutex_unlock(&g_wifi_ui_request_mutex);
}

void ui_wifi_request_set_title(const char *text)
{
    pthread_mutex_lock(&g_wifi_ui_request_mutex);
    snprintf(g_wifi_pending_title,
             sizeof(g_wifi_pending_title),
             "%s",
             text ? text : "Disconnected");
    g_wifi_pending_title_update = true;
    pthread_mutex_unlock(&g_wifi_ui_request_mutex);
}

void ui_wifi_request_set_scan_results(const char *options)
{
    pthread_mutex_lock(&g_wifi_ui_request_mutex);
    snprintf(g_wifi_pending_scan_results,
             sizeof(g_wifi_pending_scan_results),
             "%s",
             options ? options : "No Wi-Fi Found");
    g_wifi_pending_scan_results_update = true;
    pthread_mutex_unlock(&g_wifi_ui_request_mutex);
}

void ui_wifi_request_submit_phone_password(const char *password)
{
    pthread_mutex_lock(&g_wifi_ui_request_mutex);
    snprintf(g_wifi_pending_password,
             sizeof(g_wifi_pending_password),
             "%s",
             password ? password : "");
    g_wifi_pending_submit_password = true;
    pthread_mutex_unlock(&g_wifi_ui_request_mutex);
}

/* ════════════════════════════════════════════════════════
 *  build_top_panel
 * ════════════════════════════════════════════════════════ */
static void build_top_panel(lv_obj_t *scr)
{
    lv_obj_t *bg = lv_obj_create(scr);
    lv_obj_set_size(bg, SCR_W, TOP_H);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_set_style_bg_color(bg, C_BG, 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_radius(bg, 0, 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Time (single line, clipped, vertically centred) ── */
    g_lbl_time = lv_label_create(bg);
    lv_label_set_text(g_lbl_time, "-");
    lv_label_set_long_mode(g_lbl_time, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(g_lbl_time, TIME_LABEL_W);
    lv_obj_set_style_text_font(g_lbl_time, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(g_lbl_time, C_ACCENT, 0);
    lv_obj_set_style_text_align(g_lbl_time, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(g_lbl_time, LV_ALIGN_LEFT_MID, TIME_LABEL_X, 0);

    /* ── Vertical divider ── */
    lv_obj_t *div = lv_obj_create(bg);
    lv_obj_set_size(div, 1, TOP_H - 10);
    lv_obj_set_pos(div, DIV_X, 5);
    lv_obj_set_style_bg_color(div, C_DIVIDER, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_radius(div, 0, 0);

    /* ── Info rows
     *  Icon at INFO_X_ICON (86), text at INFO_X_TEXT (106).
     *  montserrat_14 icon glyph ≈ 14px wide → gap = 106-86-14 = 6px  ✓
     * ── */
#define MAKE_INFO_ROW(parent, sym, defval, row, out_lbl, long_mode, tw)      \
    do {                                                                      \
        int _y = INFO_Y_START + (row) * INFO_ROW_H;                          \
        lv_obj_t *_ico = lv_label_create(parent);                            \
        lv_label_set_text(_ico, (sym));                                       \
        lv_obj_set_style_text_font(_ico, &lv_font_montserrat_14, 0);         \
        lv_obj_set_style_text_color(_ico, C_ACCENT, 0);                      \
        lv_obj_set_pos(_ico, INFO_X_ICON, _y);                               \
        (out_lbl) = lv_label_create(parent);                                  \
        lv_label_set_text((out_lbl), (defval));                               \
        lv_obj_set_style_text_font((out_lbl), &lv_font_montserrat_12, 0);    \
        lv_obj_set_style_text_color((out_lbl), C_TEXT, 0);                   \
        lv_obj_set_pos((out_lbl), INFO_X_TEXT, _y + 2);                      \
        lv_obj_set_width((out_lbl), (tw));                                    \
        lv_label_set_long_mode((out_lbl), (long_mode));                       \
    } while(0)

    /* Row 0 – CPU Temperature */
    MAKE_INFO_ROW(bg,
                  LV_SYMBOL_WARNING,
                  "none",
                  0, g_lbl_cpu,
                  LV_LABEL_LONG_CLIP,
                  INFO_TEXT_W);

    /* Row 1 – Battery percentage (default 80%) */
    MAKE_INFO_ROW(bg,
                  LV_SYMBOL_BATTERY_FULL,
                  "none",
                  1, g_lbl_bat,
                  LV_LABEL_LONG_CLIP,
                  INFO_TEXT_W);

    /* Row 2 – IP Address (circular scroll for long strings) */
    MAKE_INFO_ROW(bg,
                  LV_SYMBOL_WIFI,
                  "none",
                  2, g_lbl_ip,
                  LV_LABEL_LONG_SCROLL_CIRCULAR,
                  INFO_TEXT_W);

#undef MAKE_INFO_ROW
}

/* ════════════════════════════════════════════════════════
 *  build_app_btn
 * ════════════════════════════════════════════════════════ */
static void build_app_btn(lv_obj_t     *parent,
                           const char   *symbol,
                           const char   *label_text,
                           lv_event_cb_t event_cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, BTN_W, BTN_H);
    lv_obj_set_style_bg_color(btn, C_BTN, 0);
    lv_obj_set_style_bg_color(btn, C_BTN_PRESS, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, BTN_RADIUS, 0);
    lv_obj_set_style_shadow_width(btn, 3, 0);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0xBB7070), 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    if (event_cb) {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }

    /* Icon circle */
    lv_obj_t *circle = lv_obj_create(btn);
    lv_obj_set_size(circle, BTN_CIRCLE_D, BTN_CIRCLE_D);
    lv_obj_align(circle, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(circle, C_ICON_CIRCLE, 0);
    lv_obj_set_style_border_width(circle, 0, 0);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(circle, 0, 0);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ico = lv_label_create(circle);
    lv_label_set_text(ico, symbol);
    lv_obj_set_style_text_font(ico, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ico, lv_color_hex(0xFFFFFF), 0); /* white icon */
    lv_obj_center(ico);

    /* App name: bottom-aligned, 5px from bottom edge */
    lv_obj_t *name = lv_label_create(btn);
    lv_label_set_text(name, label_text);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(name, C_TEXT, 0);
    lv_obj_align(name, LV_ALIGN_BOTTOM_MID, 0, -2);
}

/* ════════════════════════════════════════════════════════
 *  build_bottom_panel
 * ════════════════════════════════════════════════════════ */
static void build_bottom_panel(lv_obj_t *scr)
{
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, SCR_W, BOT_H);
    lv_obj_set_pos(cont, 0, TOP_H);
    lv_obj_set_style_bg_color(cont, C_BG, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_radius(cont, 0, 0);
    lv_obj_set_style_pad_left(cont,   BTN_PAD_OUTER, 0);
    lv_obj_set_style_pad_right(cont,  BTN_PAD_OUTER, 0);
    lv_obj_set_style_pad_top(cont,    6, 0);
    lv_obj_set_style_pad_bottom(cont, 6, 0);
    lv_obj_set_style_pad_column(cont, BTN_GAP, 0);

    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    build_app_btn(cont, LV_SYMBOL_IMAGE, "Camera", ui_event_camera_btn);
    build_app_btn(cont, LV_SYMBOL_WIFI,  "Wi-Fi",  ui_event_wifi_btn);
    build_app_btn(cont, LV_SYMBOL_POWER, "Power",  ui_event_power_btn);
}

/* ════════════════════════════════════════════════════════
 *  ui_main_screen_init
 * ════════════════════════════════════════════════════════ */
void ui_main_screen_init(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, C_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    build_top_panel(scr);
    build_bottom_panel(scr);

    lv_disp_load_scr(scr);
}

void ui_power_dialog_show(void)
{
    lv_obj_t *scr = lv_scr_act();

    if (g_power_mask && lv_obj_is_valid(g_power_mask) && lv_obj_get_parent(g_power_mask) == scr) {
        lv_obj_move_foreground(g_power_mask);
        return;
    }

    g_power_mask = lv_obj_create(scr);
    lv_obj_set_size(g_power_mask, SCR_W, SCR_H);
    lv_obj_set_pos(g_power_mask, 0, 0);
    lv_obj_set_style_bg_color(g_power_mask, C_MODAL_MASK, 0);
    lv_obj_set_style_bg_opa(g_power_mask, LV_OPA_40, 0);
    lv_obj_set_style_border_width(g_power_mask, 0, 0);
    lv_obj_set_style_radius(g_power_mask, 0, 0);
    lv_obj_set_style_pad_all(g_power_mask, 0, 0);
    lv_obj_clear_flag(g_power_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(g_power_mask, power_dialog_delete_cb, LV_EVENT_DELETE, NULL);

    g_power_dialog = lv_obj_create(g_power_mask);
    lv_obj_set_size(g_power_dialog, PWR_DLG_W, PWR_DLG_H);
    lv_obj_center(g_power_dialog);
    lv_obj_set_style_bg_color(g_power_dialog, C_MODAL_PANEL, 0);
    lv_obj_set_style_bg_opa(g_power_dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_power_dialog, 1, 0);
    lv_obj_set_style_border_color(g_power_dialog, C_MODAL_BORDER, 0);
    lv_obj_set_style_radius(g_power_dialog, 10, 0);
    lv_obj_set_style_shadow_width(g_power_dialog, 8, 0);
    lv_obj_set_style_shadow_color(g_power_dialog, lv_color_hex(0xAA7070), 0);
    lv_obj_set_style_shadow_opa(g_power_dialog, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(g_power_dialog, 0, 0);
    lv_obj_clear_flag(g_power_dialog, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *close_btn = lv_btn_create(g_power_dialog);
    lv_obj_set_size(close_btn, 22, 22);
    lv_obj_set_pos(close_btn, 4, 4);
    lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(close_btn, C_BACK_BTN, 0);
    lv_obj_set_style_bg_color(close_btn, C_BACK_BTN_PR, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(close_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(close_btn, 0, 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0);
    lv_obj_set_style_pad_all(close_btn, 0, 0);
    lv_obj_add_event_cb(close_btn, ui_event_power_close_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(close_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(close_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(close_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(close_label);

    lv_obj_t *message = lv_label_create(g_power_dialog);
    lv_label_set_text(message, "Do you want to reboot or shutdown?");
    lv_obj_set_width(message, PWR_DLG_W - 38);
    lv_label_set_long_mode(message, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(message, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(message, C_TEXT, 0);
    lv_obj_set_style_text_align(message, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(message, LV_ALIGN_TOP_MID, 10, 7);

    build_power_action_btn(g_power_dialog, "Reboot", 7, ui_event_power_reboot_btn);
    build_power_action_btn(g_power_dialog, "Shutdown", 75, ui_event_power_shutdown_btn);
}

void ui_power_dialog_close(void)
{
    if (g_power_mask && lv_obj_is_valid(g_power_mask)) {
        lv_obj_del(g_power_mask);
    }
}

/* ════════════════════════════════════════════════════════
 *  ui_init
 * ════════════════════════════════════════════════════════ */
void ui_init(void)
{
    ui_main_screen_init();

    if(g_wifi_ui_dispatch_timer == NULL)
    {
        g_wifi_ui_dispatch_timer = lv_timer_create(wifi_ui_dispatch_timer_cb, 20, NULL);
    }

    /* Start the data thread */
    data_create_thread();
}

/* ════════════════════════════════════════════════════════
 *  Runtime update API
 * ════════════════════════════════════════════════════════ */
void ui_main_set_time(const char *t)    { lv_label_set_text(g_lbl_time, t); }
void ui_main_set_cpu(const char *v)     { lv_label_set_text(g_lbl_cpu,  v); }
void ui_main_set_battery(const char *v) { lv_label_set_text(g_lbl_bat,  v); }
void ui_main_set_ip(const char *v)      { lv_label_set_text(g_lbl_ip,   v); }