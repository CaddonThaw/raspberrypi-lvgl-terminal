#include "threads_conf.h"

bool cv_is_running = false;
bool cv_ret = false;
pthread_t thread_cv;

void cv_create_thread(void) 
{
    if(!cv_is_running) 
    {
        cv_is_running = true;
        pthread_create(&thread_cv, NULL, cv_thread, NULL);
    }
}

void cv_destroy_thread(void) 
{
    if(cv_is_running)
    {
        cv_is_running = false;
        pthread_join(thread_cv, NULL);
    }
}

void* cv_thread(void* arg) 
{
    cv_ret = cv_init(); 
    while(1) 
    {
        if(!cv_is_running || !cv_ret)break;
        cv_loop();
        usleep(10000); 
    }
    cv_deinit(cv_ret);
    return NULL;
}