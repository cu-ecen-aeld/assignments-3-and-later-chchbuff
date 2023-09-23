#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

#define SUCCESS   (0)
#define FAILURE   (-1)
#define CONVERT_MS_TO_US(value)  ((1000)*value)

void* threadfunc(void* thread_param)
{
    int return_value = FAILURE;
    
    if (NULL != thread_param)
    {
        // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
        // hint: use a cast like the one below to obtain thread arguments from your parameter
        struct thread_data* thread_func_args = (struct thread_data *) thread_param;
        usleep(CONVERT_MS_TO_US(thread_func_args->wait_to_obtain_ms));
    
        return_value = pthread_mutex_lock(thread_func_args->thread_mutex);
        if (SUCCESS != return_value)
        {
            ERROR_LOG("locking thread mutex return_value: %d", return_value);
            thread_func_args->thread_complete_success = false;
        }
        else
        {
            usleep(CONVERT_MS_TO_US(thread_func_args->wait_to_release_ms));
            return_value = pthread_mutex_unlock(thread_func_args->thread_mutex);
            if (SUCCESS != return_value)
            {
                ERROR_LOG("unlocking thread mutex return_value: %d", return_value);
                thread_func_args->thread_complete_success = false;
            }
            else
            {
                thread_func_args->thread_complete_success = true;
            }
        }
    }
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    bool status = false;
    int return_value = FAILURE;
    
    if ((NULL == thread) && (NULL == mutex))
    {
        return false;
    }

    thread_data_t *thread_data = (thread_data_t *)malloc(sizeof(thread_data_t));
    if (NULL == thread_data)
    {
        ERROR_LOG("Allocating memory to thread_data_t: %s", strerror(errno));
        status = false;
    }
    else
    {
        thread_data->thread_mutex = mutex;
        thread_data->wait_to_obtain_ms = wait_to_obtain_ms;
        thread_data->wait_to_release_ms = wait_to_release_ms;
        thread_data->thread_complete_success = false;
        return_value = pthread_create(thread, NULL, 
                                     (void *)threadfunc,
                                     (void *)thread_data);
        if (SUCCESS != return_value)
        {
            ERROR_LOG("Creating thread return_value: %d", return_value);
            if (NULL != thread_data)
            {
                free(thread_data);
                thread_data = NULL;
            }
            thread = NULL;
            status = false;
        }
        else
        {
            DEBUG_LOG("start_thread_obtaining_mutex created successfully");
            status = true;
        }
    }
    return status;
}

