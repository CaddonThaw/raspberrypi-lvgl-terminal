#include "threads_conf.h"

pthread_t thread_data;

void data_create_thread(void) 
{
    pthread_create(&thread_data, NULL, data_thread, NULL);
}

void* data_thread(void* arg) 
{
    while(1) 
    {
        data_loop();
        usleep(1000000); 
    }
    return NULL;
}