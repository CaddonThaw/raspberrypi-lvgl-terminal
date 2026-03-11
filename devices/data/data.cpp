#include "data.h"
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "lvgl/lvgl.h"
#include "ui/ui.h"

void data_loop(void)
{
    time_t current_time;
    time_t china_time;
    struct tm *time_info;
    char time_string[100];
    char cpu_temp_string[32];
    char ip_string[64];
    FILE *fp;
    int milli_c;
    struct ifaddrs *ifaddr = NULL;
    struct ifaddrs *ifa = NULL;
    
    time(&current_time);
    china_time = current_time + 8 * 60 * 60;
    time_info = gmtime(&china_time);

    snprintf(time_string, sizeof(time_string), "%02d:%02d", time_info->tm_hour, time_info->tm_min);

    fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if(fp == NULL)
    {
        snprintf(cpu_temp_string, sizeof(cpu_temp_string), "none");
    }
    else
    {
        milli_c = 0;
        if(fscanf(fp, "%d", &milli_c) == 1)
        {
            snprintf(cpu_temp_string, sizeof(cpu_temp_string), "%.1f\xc2\xb0""C", milli_c / 1000.0f);
        }
        else
        {
            snprintf(cpu_temp_string, sizeof(cpu_temp_string), "none");
        }
        fclose(fp);
    }

    snprintf(ip_string, sizeof(ip_string), "none");
    if(getifaddrs(&ifaddr) != -1)
    {
        for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if(ifa->ifa_addr == NULL)
            {
                continue;
            }

            if(ifa->ifa_addr->sa_family != AF_INET)
            {
                continue;
            }

            if((ifa->ifa_flags & IFF_LOOPBACK) != 0)
            {
                continue;
            }

            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            if(inet_ntop(AF_INET, &addr->sin_addr, ip_string, sizeof(ip_string)) != NULL)
            {
                break;
            }
        }
        freeifaddrs(ifaddr);
    }

    ui_main_set_time(time_string);
    ui_main_set_cpu(cpu_temp_string);
    ui_main_set_ip(ip_string);
}