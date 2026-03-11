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
    lv_obj_set_style_bg_color(scr, C_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    build_back_button(scr);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Wi-Fi");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, C_ACCENT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    /* ── user adds widgets here ── */
    return scr;
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
    lv_label_set_text(g_lbl_time, "23:28");
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
                  "25\xc2\xb0""C",
                  0, g_lbl_cpu,
                  LV_LABEL_LONG_CLIP,
                  INFO_TEXT_W);

    /* Row 1 – Battery percentage (default 80%) */
    MAKE_INFO_ROW(bg,
                  LV_SYMBOL_BATTERY_FULL,
                  "80%",
                  1, g_lbl_bat,
                  LV_LABEL_LONG_CLIP,
                  INFO_TEXT_W);

    /* Row 2 – IP Address (circular scroll for long strings) */
    MAKE_INFO_ROW(bg,
                  LV_SYMBOL_WIFI,
                  "192.168.255.255",
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

/* ════════════════════════════════════════════════════════
 *  Runtime update API
 * ════════════════════════════════════════════════════════ */
void ui_main_set_time(const char *t)    { lv_label_set_text(g_lbl_time, t); }
void ui_main_set_cpu(const char *v)     { lv_label_set_text(g_lbl_cpu,  v); }
void ui_main_set_battery(const char *v) { lv_label_set_text(g_lbl_bat,  v); }
void ui_main_set_ip(const char *v)      { lv_label_set_text(g_lbl_ip,   v); }