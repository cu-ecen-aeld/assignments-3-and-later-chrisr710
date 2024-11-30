#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)  //this function accepts any pointer as an argument!
{
    printf("NEW thread:%ld\n\n",pthread_self());
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    thread_func_args->thread_complete_success=false;
    //printf("%d",thread_func_args->sleep2);
    //wait
    //pthread_mutex_unlock((thread_func_args->mymutex));
    int sleep_first=0;
    int sleep_second=0;
    sleep_first=(thread_func_args->sleep1 * 1000);
    sleep_second=(thread_func_args->sleep1 * 1000);
    DEBUG_LOG("sleeping %d",sleep_first);
    usleep(sleep_first);    
    DEBUG_LOG("locking mutex");
    //DEBUG_LOG("The address of the mutex I will lock is: %p\n", (void *)thread_func_args->mymutex);
    pthread_mutex_lock(thread_func_args->mymutex);
    DEBUG_LOG("sleeping %d",sleep_second);
  
    //wait
    usleep(sleep_second);
    //release mutex
    pthread_mutex_unlock(thread_func_args->mymutex);
    DEBUG_LOG("released the mutex");
    thread_func_args->thread_complete_success=true;
    //DEBUG_LOG("The address of the struct I am writing to and returning is: %p\n", (void *)thread_func_args);
    
   return(thread_func_args);
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
    printf("Main thread:%ld\n\n",pthread_self());
    //pthread_t thread_id; //id for newly created thread.    
    struct thread_data *myThreadData = malloc(sizeof(*myThreadData)); //allocate a pointer by mallocing the size of a thread_data struct.
    if (myThreadData != NULL){ //if malloc failed, this will be null)
                myThreadData->thread_complete_success=false;
    		myThreadData->sleep1=wait_to_obtain_ms;
    		myThreadData->mymutex=mutex;
                //pthread_mutex_init(myThreadData->mymutex, NULL);
                //pthread_mutex_unlock((myThreadData->mymutex));
                DEBUG_LOG("The address of the mutex is: %p\n", (void *)mutex);
    		myThreadData->sleep2=wait_to_release_ms;
                //int rc=0;
                int rc=pthread_create(thread,NULL,threadfunc,myThreadData);
                //*thread=thread_id;
                //threadfunc(myThreadData); //run threadfunc with the data.
		DEBUG_LOG("NOW I AM RETURNING:");
                if (rc == 0){
                
                DEBUG_LOG("returning ptr to myThreadData from starting function...");
                //free(myThreadData);
                myThreadData->thread_complete_success=true;
                DEBUG_LOG("The address I am returning is: %p\n", (void *)myThreadData);
		DEBUG_LOG("The thread id is:%ld",*thread);
		return(myThreadData);
                
		}
	}
    //if we got here, something went wrong
    DEBUG_LOG("RETURNING FALSE");
    myThreadData->thread_complete_success=false;
    return(myThreadData);
	}


