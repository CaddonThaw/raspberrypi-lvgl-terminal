#ifndef DATE_H
#define DATE_H

#include <unistd.h>
#include <stdio.h>
#include <cstdint>

void data_loop(void);
void set_fan(int enabled);
void set_led1(int enabled);
void set_led2(int enabled);
void set_buzzer(int enabled);

#endif 