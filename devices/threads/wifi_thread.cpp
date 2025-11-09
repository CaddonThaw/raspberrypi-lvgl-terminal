#include "threads_conf.h"

bool wifi_thread_is_running = false;
bool wifi_thread_is_flush = false;
bool wifi_thread_is_connect = false;
char wifi_ssid[256],wifi_pass[256];
pthread_t thread_wifi;

void wifi_create_thread(void) 
{
    if(!wifi_thread_is_running) 
    {
        wifi_thread_is_running = true;
        pthread_create(&thread_wifi, NULL, wifi_thread, NULL);
    }
}

void wifi_destroy_thread(void) 
{
    if(wifi_thread_is_running)
    {
        wifi_thread_is_running = false;
        pthread_join(thread_wifi, NULL);
    }
}

void* wifi_thread(void* arg) 
{
    while(1) 
    {
        if(!wifi_thread_is_flush)
        {
            if(wifi_scan() == 0)wifi_thread_is_connect = true;
            wifi_thread_is_flush = true;
        }
        if(!wifi_thread_is_connect)
        {
            wifi_connect((const char*)wifi_ssid, (const char*)wifi_pass);
            wifi_get_clear();
            wifi_thread_is_connect = true;
        }
        if(!wifi_thread_is_running)break;
        usleep(10000); 
    }
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
    strcpy(wifi_ssid, ssid);
}

void wifi_get_pass(const char *pass)
{
    strcpy(wifi_pass, pass);
}

void wifi_get_clear(void)
{
    memset(wifi_ssid, '\0', sizeof(wifi_ssid));
    memset(wifi_pass, '\0', sizeof(wifi_pass));
}