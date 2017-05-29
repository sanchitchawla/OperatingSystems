#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "threadpool.h"

// _threadpool is the internal threadpool structure that is
// cast to type "threadpool" before it given out to callers
typedef struct queue
{
  void (*routine) (void*); // function pointer
  void *arg;
  struct queue *next;
} queue;

typedef struct _threadpool_st {
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


void *worker_thread(void *p) {
  _threadpool* pool = (_threadpool*)p;
  queue* que;

  while (1) {
    // Lock taken to wait on conditional var
    pthread_mutex_lock(&(pool->lock));

    // if size if 0 unlock the mutex so other processes can use it
    while(pool->size == 0){
      if (pool->isShutDown){
          pthread_mutex_unlock(&(pool->lock));
          pthread_exit(NULL);
      }

      pthread_mutex_unlock(&(pool->lock));

      // wait till its not empty
      pthread_cond_wait(&(pool->not_empty), &(pool->lock));

      if (pool->isShutDown){
          pthread_mutex_unlock(&(pool->lock));
          pthread_exit(NULL);
      }
    }
    // End of while loop
    que = pool->head;
    pool->size--;

    if (pool->size == 0){
      pool->head = pool->tail = NULL;
    }
    else{
      pool->head = que->next;
    }

    if (pool->size == 0 && !pool->isShutDown){
      // now its empty so we send a signal
      pthread_cond_signal(&(pool->empty));
    }

    pthread_mutex_unlock(&(pool->lock));
    // Call the function with the arguments
    (*(que->routine)) (que->arg);
    // free everything you malloc
    free(que);

  }
}


threadpool create_threadpool(int num_threads_in_pool) {
  _threadpool *pool;

  // sanity check the argument
  if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
    return NULL;

  pool = (_threadpool *) malloc(sizeof(_threadpool));
  if (pool == NULL) {
    fprintf(stderr, "Error whilst creating threadpool\n");
    return NULL;
  }

  pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads_in_pool);

  if (!pool->threads){
    fprintf(stderr, "Error whilst creating threadpool\n");
    return NULL;
  }

  pool->size        = 0;
  pool->isNotAccept = 0;
  pool->isShutDown  = 0;
  pool->head        = NULL;
  pool->tail        = NULL;
  pool->num_of_threads = num_threads_in_pool;

  if (pthread_cond_init(&pool->empty, NULL)){
    fprintf(stderr, "CV initiation error\n");
    return NULL;
  }
  if (pthread_mutex_init(&pool->lock, NULL)){
    fprintf(stderr, "Mutex error\n");
    return NULL;
  }
  if (pthread_cond_init(&pool->not_empty, NULL)){
    fprintf(stderr, "CV initiation error\n");
    return NULL;
  }

  for (int i = 0; i < pool->num_of_threads;i++){
    if (pthread_create(&(pool->threads[i]), NULL, worker_thread, pool)){ // on success returns 0 
      fprintf(stderr, "Thread couldn't initialize\n");
      return NULL;
    }
  }
  return (threadpool) pool;
}


void dispatch(threadpool from_me, dispatch_fn dispatch_to_here,
	      void *arg) {

  _threadpool *pool = (_threadpool *) from_me;
  queue* que;

  // initialize the queue
  que = (queue*)malloc(sizeof(queue));
  if (que == NULL){
    fprintf(stderr, "Error in creating the queue in dispatch function\n");
  }

  que->routine = dispatch_to_here;
  que->arg = arg;
  que->next = NULL;

  // take hold of the mutex
  pthread_mutex_lock(&(pool->lock));

  if (pool->size == 0){
    // Initializing queue with one item
    pool->head = que;
    pool->tail = que;
    // sending a signal that the pool is not empty
    pthread_cond_signal(&(pool->not_empty));
  }
  else{
    // add to the end
    pool->tail->next = que;
    pool->tail = que;
  }

  // for (int i = 0; i < pool->num_of_threads;i++){
  //   if (pthread_create(&(pool->threads[i]), NULL, worker_thread, pool)){ // on success returns 0 
  //     fprintf(stderr, "Thread couldn't initialize\n");
  //     return NULL;
  //   }
  // }
  // increment the size since we're adding
  pool->size++;

  // send a signal to unlock
  pthread_mutex_unlock(&(pool->lock));
}

void destroy_threadpool(threadpool destroyme) {
  _threadpool *pool = (_threadpool *) destroyme;

  void* place;
  // send a signal to lock 
  pthread_mutex_lock(&(pool->lock));
  pool->isNotAccept = 1;
  while(pool->size != 0){
    // wait until its empty
    pthread_cond_wait(&(pool->empty), &(pool->lock));
  }

  pthread_cond_broadcast(&(pool->not_empty));
  pool->isShutDown = 1;

  // unlock the mutex
  pthread_mutex_unlock(&(pool->lock));

  // kill each thread
  int i = 0;
  while(i < pool->num_of_threads){
    pthread_cond_broadcast(&(pool->not_empty));
    pthread_join(pool->threads[i], &place);
    i++;
  }
  // free the threads
  free(pool->threads);
  pthread_mutex_destroy(&(pool->lock));
  pthread_cond_destroy(&(pool->empty));
  pthread_cond_destroy(&(pool->not_empty));

}