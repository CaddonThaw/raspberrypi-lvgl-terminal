#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/* —— UI init —— */
void ui_init(void);

/* ── Screen factory functions (used by ui_event.c) ── */
lv_obj_t *ui_camera_screen_create(void);
lv_obj_t *ui_wifi_screen_create(void);

/* ── Main screen ── */
/**
 * Create and load the main screen.
 * Call once after lv_init() + display driver registration.
 * Also called internally when returning from sub-screens.
 */
void ui_main_screen_init(void);

/* ── Power dialog ── */
void ui_power_dialog_show(void);
void ui_power_dialog_close(void);

/* ── Wi-Fi dialog ── */
void ui_wifi_qr_dialog_show(void);
void ui_wifi_qr_dialog_close(void);
void ui_wifi_loading_show(void);
void ui_wifi_loading_close(void);
void ui_wifi_set_scan_results(const char *options);
void ui_wifi_submit_phone_password(const char *password);
const char *ui_wifi_get_selected_ssid(void);
const char *ui_wifi_get_submitted_password(void);

/* ── Runtime data update ── */
void ui_main_set_time(const char *t);      /* e.g. "23:28"           */
void ui_main_set_cpu(const char *v);       /* e.g. "42\xc2\xb0""C"  */
void ui_main_set_battery(const char *v);   /* e.g. "1800mA"          */
void ui_main_set_ip(const char *v);        /* e.g. "192.168.255.255" */

#ifdef __cplusplus
}
#endif

#endif /* UI_H */