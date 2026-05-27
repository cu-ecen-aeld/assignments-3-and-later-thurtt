#include "threading.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data * func_args = (struct thread_data *)thread_param;

    usleep(func_args->wait_to_obtain_ms * 1000);
    int lock_status = pthread_mutex_lock(func_args->mutex);
    if( lock_status != 0 )
    {
        func_args->thread_complete_success = false;
        return thread_param;
    }

    usleep(func_args->wait_to_release_ms * 1000);
    pthread_mutex_unlock(func_args->mutex);

    func_args->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data * tdata = (struct thread_data *)malloc(sizeof(struct thread_data));

    tdata->mutex = mutex;
    tdata->wait_to_obtain_ms = wait_to_obtain_ms;
    tdata->wait_to_release_ms = wait_to_release_ms;

    int thread_result = pthread_create(thread, NULL, threadfunc, tdata);

    if (thread_result != 0)
    {
        ERROR_LOG("Error creating thread: %d", thread_result);
        free(tdata);
        return false;
    }

    return true;
}
