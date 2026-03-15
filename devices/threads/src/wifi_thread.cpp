#include "../threads_conf.h"
#include "ui/ui.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

bool wifi_thread_is_running = false;
bool wifi_thread_is_flush = false;
bool wifi_thread_is_connect = true;
char wifi_ssid[256],wifi_pass[256];
pthread_t thread_wifi;

void wifi_create_thread(void) 
{
    if(!wifi_thread_is_running) 
    {
        wifi_thread_is_flush = false;
        wifi_thread_is_connect = true;
        wifi_ssid[0] = '\0';
        wifi_pass[0] = '\0';
        wifi_thread_is_running = true;
        pthread_create(&thread_wifi, NULL, wifi_thread, NULL);
    }
}

void wifi_destroy_thread(void) 
{
    if(wifi_thread_is_running) 
    {
        ui_wifi_qr_dialog_close();
        ui_wifi_loading_close();
        wifi_thread_is_running = false;
        pthread_join(thread_wifi, NULL);
        wifi_thread_is_flush = false;
        wifi_thread_is_connect = true;
        wifi_get_clear();
    }
}

void* wifi_thread(void* arg) 
{
    (void)arg;

    wifi_portal_start();

    while(wifi_thread_is_running) 
    {
        wifi_portal_poll();

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

            ui_wifi_loading_close();

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