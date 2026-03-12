/**
 * ui_event.c
 * All LVGL event callbacks for the demo UI.
 */

#include "ui_event.h"
#include "ui.h"
#include "lvgl/lvgl.h"
#include "devices/threads/threads_conf.h"

/* ════════════════════════════════════════════════════════
 *  Internal: load next screen, async-delete old screen
 * ════════════════════════════════════════════════════════ */
static void screen_switch(lv_obj_t *next_scr, lv_obj_t *old_scr)
{
    lv_disp_load_scr(next_scr);
    lv_obj_del_async(old_scr);
}

/* ════════════════════════════════════════════════════════
 *  Back button → rebuild and return to main screen
 * ════════════════════════════════════════════════════════ */
void ui_event_back_btn(lv_event_t *e)
{
    lv_obj_t *cur_scr = lv_scr_act();
    ui_main_screen_init();       /* rebuild main screen, calls lv_disp_load_scr */
    lv_obj_del_async(cur_scr);   /* free sub-screen RAM                         */

    /* Restart the data thread */
    data_create_thread();
}

/* ════════════════════════════════════════════════════════
 *  Camera button → navigate to camera screen
 * ════════════════════════════════════════════════════════ */
void ui_event_camera_btn(lv_event_t *e)
{
    lv_obj_t *old_scr = lv_scr_act();
    lv_obj_t *cam_scr = ui_camera_screen_create();
    screen_switch(cam_scr, old_scr);

    /* Stop the data thread */
    data_destroy_thread(); 
}

/* ════════════════════════════════════════════════════════
 *  Wi-Fi button → navigate to Wi-Fi screen
 * ════════════════════════════════════════════════════════ */
void ui_event_wifi_btn(lv_event_t *e)
{
    lv_obj_t *old_scr = lv_scr_act();
    lv_obj_t *wifi_scr = ui_wifi_screen_create();
    screen_switch(wifi_scr, old_scr);

    /* Stop the data thread */
    data_destroy_thread(); 
}

/* ════════════════════════════════════════════════════════
 *  Power button → user fills in action
 * ════════════════════════════════════════════════════════ */
void ui_event_power_btn(lv_event_t *e)
{
    /* Show the power dialog */
    ui_power_dialog_show();
}

void ui_event_power_close_btn(lv_event_t *e)
{
    /* Close the power dialog */
    ui_power_dialog_close();
}

void ui_event_power_reboot_btn(lv_event_t *e)
{
    /* Close the power dialog */
    ui_power_dialog_close();

    /* Reboot action */
    power_reboot();
}

void ui_event_power_shutdown_btn(lv_event_t *e)
{
    /* Close the power dialog */
    ui_power_dialog_close();

    /* Shutdown action */
    power_shutdown();
}