#include "threads_conf.h"

bool message_is_running = false;
pthread_t thread_message;

void message_create_thread(void) 
{
    if(!message_is_running) 
    {
        message_is_running = true;
        pthread_create(&thread_message, NULL, message_thread, NULL);
    }
}

void message_destroy_thread(void) 
{
    if(message_is_running)
    {
        message_is_running = false;
        pthread_join(thread_message, NULL);
    }
}

void* message_thread(void* arg) 
{
    tm7711_init(); 
    while(1) 
    {
        if(!message_is_running)break;
        tm7711_loop();
        usleep(10000); 
    }
    return NULL;
}