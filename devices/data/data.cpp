#include "data.h"
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <mutex>
#include <wiringPiI2C.h>
#include "lv_drv_conf.h"
#include "lvgl/lvgl.h"
#include "ui/ui.h"

namespace {

    constexpr int REG_CONFIG = 0x00;
    constexpr int REG_SHUNTVOLTAGE = 0x01;
    constexpr int REG_BUSVOLTAGE = 0x02;
    constexpr int REG_CALIBRATION = 0x05;
    constexpr uint16_t INA219_CALIBRATION_32V_2A = 4096;
    constexpr uint16_t INA219_CONFIG_32V_2A =
        (0x01 << 13) | (0x03 << 11) | (0x0D << 7) | (0x0D << 3) | 0x07;
    constexpr float FAN_ON_TEMP_C = 25.0f;
    constexpr float BATTERY_LOW_VOLTAGE = 7.4f;
    constexpr float BATTERY_FULL_VOLTAGE = 8.4f;
    constexpr uint64_t LOW_BATTERY_WARN_INTERVAL_MS = 30000;
    constexpr uint64_t LOW_BATTERY_BEEP_DURATION_MS = 150;
    constexpr uint64_t LOW_BATTERY_BEEP_PERIOD_MS = 1000;
    constexpr uint64_t LOW_BATTERY_BEEP_COUNT = 3;
    std::once_flag gpio_init_once;
    bool gpio_driver_ready = false;

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
            fd = wiringPiI2CSetupInterface(BATTERY_I2C_DEV, BATTERY_I2C_ADDR);
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

void set_fan(int enabled)
{
    std::call_once(gpio_init_once, []() {
        if(wiringPiSetupGpio() == -1)
        {
            gpio_driver_ready = false;
            return;
        }

        pinMode(FAN_PIN, OUTPUT);
        digitalWrite(FAN_PIN, LOW);
        pinMode(LED1_PIN, OUTPUT);
        digitalWrite(LED1_PIN, LOW);
        pinMode(LED2_PIN, OUTPUT);
        digitalWrite(LED2_PIN, LOW);
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW);
        gpio_driver_ready = true;
    });

    if(!gpio_driver_ready)
    {
        return;
    }

    digitalWrite(FAN_PIN, enabled ? HIGH : LOW);
}

void set_led1(int enabled)
{
    std::call_once(gpio_init_once, []() {
        if(wiringPiSetupGpio() == -1)
        {
            gpio_driver_ready = false;
            return;
        }

        pinMode(FAN_PIN, OUTPUT);
        digitalWrite(FAN_PIN, LOW);
        pinMode(LED1_PIN, OUTPUT);
        digitalWrite(LED1_PIN, LOW);
        pinMode(LED2_PIN, OUTPUT);
        digitalWrite(LED2_PIN, LOW);
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW);
        gpio_driver_ready = true;
    });

    if(!gpio_driver_ready)
    {
        return;
    }

    digitalWrite(LED1_PIN, enabled ? HIGH : LOW);
}

void set_led2(int enabled)
{
    std::call_once(gpio_init_once, []() {
        if(wiringPiSetupGpio() == -1)
        {
            gpio_driver_ready = false;
            return;
        }

        pinMode(FAN_PIN, OUTPUT);
        digitalWrite(FAN_PIN, LOW);
        pinMode(LED1_PIN, OUTPUT);
        digitalWrite(LED1_PIN, LOW);
        pinMode(LED2_PIN, OUTPUT);
        digitalWrite(LED2_PIN, LOW);
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW);
        gpio_driver_ready = true;
    });

    if(!gpio_driver_ready)
    {
        return;
    }

    digitalWrite(LED2_PIN, enabled ? HIGH : LOW);
}

void set_buzzer(int enabled)
{
    std::call_once(gpio_init_once, []() {
        if(wiringPiSetupGpio() == -1)
        {
            gpio_driver_ready = false;
            return;
        }

        pinMode(FAN_PIN, OUTPUT);
        digitalWrite(FAN_PIN, LOW);
        pinMode(LED1_PIN, OUTPUT);
        digitalWrite(LED1_PIN, LOW);
        pinMode(LED2_PIN, OUTPUT);
        digitalWrite(LED2_PIN, LOW);
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW);
        gpio_driver_ready = true;
    });

    if(!gpio_driver_ready)
    {
        return;
    }

    digitalWrite(BUZZER_PIN, enabled ? HIGH : LOW);
}

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

    ui_main_request_set_time(time_string);
    ui_main_request_set_cpu(cpu_temp_string);
    ui_main_request_set_battery(battery_string);
    ui_main_request_set_ip(ip_string);

    float cpu_temp_value = atof(cpu_temp_string);
    float battery_voltage = atof(battery_string);
    struct timespec now_spec;
    uint64_t now_ms;
    static uint64_t next_low_battery_warn_ms = 0;
    static uint64_t low_battery_warn_start_ms = 0;

    if(cpu_temp_value < FAN_ON_TEMP_C)
    {
        set_fan(0);
    }
    else
    {
        set_fan(1);
    }

    if(battery_voltage < 7.4f)
    {
        set_led1(0);
        set_led2(0);
    }
    else if(battery_voltage < 7.8f)
    {
        set_led1(0);
        set_led2(1);
    }
    else if(battery_voltage < 8.2f)
    {
        set_led1(1);
        set_led2(0);
    }
    else
    {
        set_led1(1);
        set_led2(1);
    }

    clock_gettime(CLOCK_MONOTONIC, &now_spec);
    now_ms = static_cast<uint64_t>(now_spec.tv_sec) * 1000ULL +
             static_cast<uint64_t>(now_spec.tv_nsec / 1000000ULL);

    if(battery_voltage < BATTERY_LOW_VOLTAGE)
    {
        if(next_low_battery_warn_ms == 0 || now_ms >= next_low_battery_warn_ms)
        {
            low_battery_warn_start_ms = now_ms;
            next_low_battery_warn_ms = now_ms + LOW_BATTERY_WARN_INTERVAL_MS;
        }

        if(low_battery_warn_start_ms != 0)
        {
            uint64_t warn_elapsed_ms = now_ms - low_battery_warn_start_ms;
            uint64_t warn_total_ms = LOW_BATTERY_BEEP_PERIOD_MS * LOW_BATTERY_BEEP_COUNT;

            if(warn_elapsed_ms < warn_total_ms)
            {
                uint64_t phase_ms = warn_elapsed_ms % LOW_BATTERY_BEEP_PERIOD_MS;
                set_buzzer(phase_ms < LOW_BATTERY_BEEP_DURATION_MS ? 1 : 0);
            }
            else
            {
                low_battery_warn_start_ms = 0;
                set_buzzer(0);
            }
        }
        else
        {
            set_buzzer(0);
        }
    }
    else
    {
        next_low_battery_warn_ms = 0;
        low_battery_warn_start_ms = 0;
        set_buzzer(0);
    }
}