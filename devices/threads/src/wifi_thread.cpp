#include "../threads_conf.h"
#include "ui/ui.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

bool wifi_thread_is_running = false;
bool wifi_thread_stop_requested = false;
bool wifi_thread_cleanup_running = false;
bool wifi_thread_restart_requested = false;
bool wifi_thread_is_flush = false;
bool wifi_thread_is_connect = true;
bool wifi_thread_is_portal_open = false;
bool wifi_thread_is_portal_cancel = false;
char wifi_ssid[256],wifi_pass[256];
pthread_t thread_wifi;
pthread_t thread_wifi_cleanup;
pthread_mutex_t wifi_thread_state_mutex = PTHREAD_MUTEX_INITIALIZER;

static void wifi_reset_state_locked(void)
{
    wifi_thread_is_flush = false;
    wifi_thread_is_connect = true;
    wifi_thread_is_portal_open = false;
    wifi_thread_is_portal_cancel = false;
    wifi_thread_stop_requested = false;
    wifi_ssid[0] = '\0';
    wifi_pass[0] = '\0';
}

static void *wifi_cleanup_thread(void *arg)
{
    bool should_restart = false;

    (void)arg;

    pthread_join(thread_wifi, NULL);

    pthread_mutex_lock(&wifi_thread_state_mutex);
    wifi_thread_is_running = false;
    should_restart = wifi_thread_restart_requested;
    wifi_thread_restart_requested = false;
    wifi_thread_cleanup_running = false;
    wifi_reset_state_locked();
    pthread_mutex_unlock(&wifi_thread_state_mutex);

    wifi_get_clear();

    if(should_restart)
    {
        wifi_create_thread();
    }

    return NULL;
}

void wifi_create_thread(void) 
{
    pthread_mutex_lock(&wifi_thread_state_mutex);

    if(wifi_thread_is_running)
    {
        if(wifi_thread_stop_requested || wifi_thread_cleanup_running)
        {
            wifi_thread_restart_requested = true;
        }

        pthread_mutex_unlock(&wifi_thread_state_mutex);
        return;
    }

    wifi_reset_state_locked();
    wifi_thread_restart_requested = false;
    wifi_thread_cleanup_running = false;
    wifi_thread_is_running = true;
    pthread_mutex_unlock(&wifi_thread_state_mutex);

    pthread_create(&thread_wifi, NULL, wifi_thread, NULL);
}

void wifi_destroy_thread(void) 
{
    pthread_mutex_lock(&wifi_thread_state_mutex);

    if(!wifi_thread_is_running || wifi_thread_cleanup_running)
    {
        pthread_mutex_unlock(&wifi_thread_state_mutex);
        return;
    }

    wifi_thread_stop_requested = true;
    wifi_thread_cleanup_running = true;
    wifi_thread_restart_requested = false;
    pthread_mutex_unlock(&wifi_thread_state_mutex);

    pthread_create(&thread_wifi_cleanup, NULL, wifi_cleanup_thread, NULL);
    pthread_detach(thread_wifi_cleanup);
}

void* wifi_thread(void* arg) 
{
    (void)arg;
    char qr_payload[256];
    char portal_hint[160];

    wifi_portal_start();

    while(true) 
    {
        pthread_mutex_lock(&wifi_thread_state_mutex);
        bool should_stop = wifi_thread_stop_requested;
        pthread_mutex_unlock(&wifi_thread_state_mutex);

        if(should_stop)
        {
            break;
        }

        wifi_portal_poll();

        if(wifi_thread_is_portal_cancel)
        {
            wifi_stop_phone_portal();
            wifi_thread_is_portal_cancel = false;
            wifi_thread_is_portal_open = false;
        }

        if(wifi_thread_is_portal_open)
        {
            qr_payload[0] = '\0';
            portal_hint[0] = '\0';

            wifi_prepare_phone_portal(ui_wifi_get_selected_ssid(),
                                      qr_payload,
                                      sizeof(qr_payload),
                                      portal_hint,
                                      sizeof(portal_hint));
            ui_wifi_request_qr_dialog_update(qr_payload, portal_hint);
            wifi_thread_is_portal_open = false;
        }

        if(!wifi_thread_is_flush)
        {
            wifi_stop_phone_portal();
            wifi_scan();
            wifi_thread_is_flush = true;
        }

        if(!wifi_thread_is_connect)
        {
            if(wifi_connect((const char*)wifi_ssid, (const char*)wifi_pass) == 0)
            {
                printf("[wifi] connection succeeded for %s\n", wifi_ssid);
            }
            else
            {
                printf("[wifi] connection failed for %s\n", wifi_ssid);
            }

            ui_wifi_request_loading_close();

            wifi_get_clear();
            wifi_thread_is_connect = true;
        }

        usleep(10000); 
    }

    wifi_stop_phone_portal();
    wifi_portal_stop();
    return NULL;
}

void wifi_isflush(void)
{
    if(wifi_thread_is_flush)wifi_thread_is_flush = false;
}

void wifi_isconnect(void)
{
    if(wifi_thread_is_connect)wifi_thread_is_connect = false;
}

void wifi_isportalopen(void)
{
    wifi_thread_is_portal_cancel = false;
    wifi_thread_is_portal_open = true;
}

void wifi_isportalcancel(void)
{
    wifi_thread_is_portal_open = false;
    wifi_thread_is_portal_cancel = true;
}

void wifi_get_ssid(char *ssid)
{
    snprintf(wifi_ssid, sizeof(wifi_ssid), "%s", ssid ? ssid : "");
}

void wifi_get_pass(const char *pass)
{
    snprintf(wifi_pass, sizeof(wifi_pass), "%s", pass ? pass : "");
}

void wifi_get_clear(void)
{
    memset(wifi_ssid, '\0', sizeof(wifi_ssid));
    memset(wifi_pass, '\0', sizeof(wifi_pass));
}