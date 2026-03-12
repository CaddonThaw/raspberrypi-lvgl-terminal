#include "../threads_conf.h"

bool data_is_running = false;
pthread_t thread_data;

void data_create_thread(void) 
{
    if(!data_is_running) 
    {
        data_is_running = true;
        pthread_create(&thread_data, NULL, data_thread, NULL);
    }
}

void data_destroy_thread(void) 
{
    if(data_is_running)
    {
        data_is_running = false;
        pthread_join(thread_data, NULL);
    }
}

void* data_thread(void* arg) 
{
    while(1) 
    {
        if(!data_is_running)break;
        data_loop();
        usleep(100000); 
    }
    return NULL;
}