/**
 * threadpool.c
 *
 * This file will contain your implementation of a threadpool.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "threadpool.h"

// _threadpool is the internal threadpool structure that is
// cast to type "threadpool" before it given out to callers
typedef struct queue
{
  void (*routine) (void*);
  void *arg;
  struct queue *next;
} queue;

typedef struct _threadpool_st {
   // you should fill in this structure with whatever you need
  pthread_mutex_t lock;
  pthread_cond_t empty;
  pthread_cond_t not_empty;
  pthread_t* threads;
  int num_of_threads;
  int size; 
  int isShutDown;
  int isNotAccept;
  queue* head;
  queue* tail;
} _threadpool;


void *worker_thread(void *args) {
    while (1) {
        // wait for a signal
        // l
        // mark itself as busy
        // run a given function
        //
    }
}


threadpool create_threadpool(int num_threads_in_pool) {
  _threadpool *pool;

  // sanity check the argument
  if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
    return NULL;

  pool = (_threadpool *) malloc(sizeof(_threadpool));
  if (pool == NULL) {
    fprintf(stderr, "Out of memory creating a new threadpool!\n");
    return NULL;
  }

  pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads_in_pool);

  if (!pool->threads){
    fprintf(stderr, "Out of memory creating a new threadpool!\n");
    return NULL;
  }

  pool->size        = 0;
  pool->isNotAccept = 0;
  pool->isShutDown  = 0;
  pool->head        = NULL;
  pool->tail        = NULL;
  pool->num_of_threads = num_threads_in_pool;

  if (pthread_cond_init(&pool->empty), NULL){
    fprintf(stderr, "CV initiation error\n");
    return NULL;
  }
  if (pthread_mutex_init(&pool->lock, NULL)){
    fprintf(stderr, "Mutex error\n");
  }
  if (pthread_cond_init(&pool->not_empty), NULL){
    fprintf(stderr, "CV initiation error\n");
    return NULL;
  }

  for (int i = 0; i < num_threads_in_pool;i++){
    if (pthread_create(&(pool->threads[i]), NULL, worker_thread, pool)){
      fprintf(stderr, "Thread couldn't initialize\n");
      return NULL;
    }
  }
  
  return (threadpool) pool;
}


void dispatch(threadpool from_me, dispatch_fn dispatch_to_here,
	      void *arg) {
  _threadpool *pool = (_threadpool *) from_me;

  // add your code here to dispatch a thread
}

void destroy_threadpool(threadpool destroyme) {
  _threadpool *pool = (_threadpool *) destroyme;

  // add your code here to kill a threadpool
}
