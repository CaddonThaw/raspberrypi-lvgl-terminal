#ifndef UI_EVENT_H
#define UI_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/**
 * App button click callbacks.
 * Registered in ui.c, defined in ui_event.c.
 */
void ui_event_camera_btn(lv_event_t *e);
void ui_event_wifi_btn(lv_event_t *e);
void ui_event_power_btn(lv_event_t *e);
void ui_event_power_close_btn(lv_event_t *e);
void ui_event_power_reboot_btn(lv_event_t *e);
void ui_event_power_shutdown_btn(lv_event_t *e);
void ui_event_wifi_refresh_btn(lv_event_t *e);
void ui_event_wifi_connect_btn(lv_event_t *e);
void ui_event_wifi_qr_cancel_btn(lv_event_t *e);
void ui_event_camera_refresh_btn(lv_event_t *e);
void ui_event_camera_run_btn(lv_event_t *e);
void ui_event_camera_close_btn(lv_event_t *e);

/**
 * Back button callback (used by sub-screens).
 */
void ui_event_back_btn(lv_event_t *e);

#ifdef __cplusplus
}
#endif

#endif /* UI_EVENT_H */