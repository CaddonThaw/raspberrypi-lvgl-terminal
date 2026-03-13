#include "data.h"
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <wiringPiI2C.h>
#include "lvgl/lvgl.h"
#include "ui/ui.h"

namespace {

    constexpr int INA219_ADDR = 0x42;
    constexpr int REG_CONFIG = 0x00;
    constexpr int REG_SHUNTVOLTAGE = 0x01;
    constexpr int REG_BUSVOLTAGE = 0x02;
    constexpr int REG_CALIBRATION = 0x05;
    constexpr uint16_t INA219_CALIBRATION_32V_2A = 4096;
    constexpr uint16_t INA219_CONFIG_32V_2A =
        (0x01 << 13) | (0x03 << 11) | (0x0D << 7) | (0x0D << 3) | 0x07;

    uint16_t byte_swap_u16(uint16_t value)
    {
        return static_cast<uint16_t>((value >> 8) | (value << 8));
    }

    bool ina219_write_u16(int fd, int reg, uint16_t value)
    {
        return wiringPiI2CWriteReg16(fd, reg, byte_swap_u16(value)) == 0;
    }

    bool ina219_read_u16(int fd, int reg, uint16_t *value)
    {
        int result = wiringPiI2CReadReg16(fd, reg);

        if(result < 0)
        {
            return false;
        }

        *value = byte_swap_u16(static_cast<uint16_t>(result));
        return true;
    }

    void get_battery_voltage(char *buffer, size_t buffer_size)
    {
        static int fd = -1;
        uint16_t bus_voltage_raw;
        float battery_voltage;

        snprintf(buffer, buffer_size, "none");

        if(fd < 0)
        {
            fd = wiringPiI2CSetup(INA219_ADDR);
            if(fd < 0)
            {
                return;
            }

            if(!ina219_write_u16(fd, REG_CALIBRATION, INA219_CALIBRATION_32V_2A))
            {
                return;
            }

            if(!ina219_write_u16(fd, REG_CONFIG, INA219_CONFIG_32V_2A))
            {
                return;
            }
        }

        if(!ina219_read_u16(fd, REG_BUSVOLTAGE, &bus_voltage_raw))
        {
            return;
        }

        battery_voltage = static_cast<float>(bus_voltage_raw >> 3) * 0.004f;

        snprintf(buffer, buffer_size, "%.2fV", battery_voltage);
    }

}  // namespace

void data_loop(void)
{
    time_t current_time;
    time_t china_time;
    struct tm *time_info;
    char time_string[100];
    char cpu_temp_string[32];
    char ip_string[64];
    char battery_string[32];
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

    get_battery_voltage(battery_string, sizeof(battery_string));

    ui_main_set_time(time_string);
    ui_main_set_cpu(cpu_temp_string);
    ui_main_set_battery(battery_string);
    ui_main_set_ip(ip_string);
}