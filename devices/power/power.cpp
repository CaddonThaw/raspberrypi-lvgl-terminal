#include "power.h"
#include "../threads/threads_conf.h"
#include "../../lv_drv_conf.h"
#include <stdlib.h>
#include <sys/stat.h>

void power_shutdown(void)
{
    data_destroy_thread();
    digitalWrite(PIN_BLK, 0);
    set_fan(0);
    set_led1(0);
    set_led2(0);
    set_buzzer(0);
    sync();
    system("/sbin/shutdown -h now");
}

void power_reboot(void)
{
    data_destroy_thread();
    digitalWrite(PIN_BLK, 0);
    set_fan(0);
    set_led1(0);
    set_led2(0);
    set_buzzer(0);
    sync();
    system("/sbin/reboot");
}

