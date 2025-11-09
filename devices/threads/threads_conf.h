#ifndef THREADS_CFG_H
#define THREADS_CFG_H

#include <pthread.h>
#include <stdbool.h>   
#include <unistd.h>   
#include "devices/opencv/cv.h"
#include "devices/wifi/wifi.h"
#include "devices/tm7711/tm7711.h"
#include "devices/power/power.h"

void cv_create_thread(void);
void cv_destroy_thread(void);
void* cv_thread(void* arg);

void wifi_create_thread(void);
void wifi_destroy_thread(void); 
void* wifi_thread(void* arg);
void wifi_isflush(void);
void wifi_isconnect(void);
void wifi_get_ssid(char *ssid);
void wifi_get_pass(const char *pass);
void wifi_get_clear(void);

void message_create_thread(void);
void message_destroy_thread(void); 
void* message_thread(void* arg); 

#endif // THREAD_H