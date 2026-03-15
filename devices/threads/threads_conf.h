#ifndef THREADS_CFG_H
#define THREADS_CFG_H

#include <pthread.h>
#include <stdbool.h>   
#include <unistd.h>   
#include "devices/power/power.h"
#include "devices/data/data.h"
#include "devices/wifi/wifi.h"
#include "devices/camera/camera.h"

void data_create_thread(void);
void data_destroy_thread(void);
void* data_thread(void* arg);

void wifi_create_thread(void);
void wifi_destroy_thread(void); 
void* wifi_thread(void* arg);
void wifi_isflush(void);
void wifi_isconnect(void);
void wifi_isportalopen(void);
void wifi_isportalcancel(void);
void wifi_get_ssid(char *ssid);
void wifi_get_pass(const char *pass);
void wifi_get_clear(void);

void camera_create_thread(void);
void camera_destroy_thread(void);
void* camera_thread(void* arg);
void camera_isrefresh(void);
void camera_isrun(void);
void camera_isstop(void);
void camera_get_script(const char *script_name);

#endif // THREAD_H