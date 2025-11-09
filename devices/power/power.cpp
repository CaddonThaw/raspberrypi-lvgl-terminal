#include "power.h"
#include "../../lv_drv_conf.h"
#include <stdlib.h>
#include <sys/stat.h>

void power_shutdown(void)
{
    digitalWrite(PIN_BLK, 0);
    sync();
    system("/sbin/shutdown -h now");
}

void power_reboot(void)
{
    digitalWrite(PIN_BLK, 0);
    sync();
    system("/sbin/reboot");
}

