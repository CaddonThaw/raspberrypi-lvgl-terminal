#ifndef THREADS_CFG_H
#define THREADS_CFG_H

#include <pthread.h>
#include <stdbool.h>   
#include <unistd.h>   
// #include "devices/power/power.h"
#include "devices/data/data.h"

void data_create_thread(void);
void data_destroy_thread(void);
void* data_thread(void* arg);

#endif // THREAD_H