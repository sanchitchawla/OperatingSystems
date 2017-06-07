#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "threadpool.h"

// // _threadpool is the internal threadpool structure that is
// // cast to type "threadpool" before it given out to callers
// typedef struct queue
// {
//   void (*func) (void*); // function pointer
//   void *arg;
//   struct queue *next;
// } queue;

// typedef struct _threadpool_st {
//   pthread_mutex_t lock;
//   pthread_cond_t empty;
//   pthread_cond_t not_empty;
//   pthread_t* threads;
//   int num_of_threads;
//   int threads_left;
//   int size; 
//   int isShutDown;
//   int isNotAccept;
//   queue* head;
//   queue* tail;
// } _threadpool;


// void *worker_thread(void *p) {
//   _threadpool* pool = (_threadpool*)p;
//   queue* que;

//   while (1) {
//     // Lock taken to wait on conditional var
//     pthread_mutex_lock(&(pool->lock));
//     // wait till its not empty
//     if (pool->size == 0){
//       pthread_cond_wait(&(pool->not_empty), &(pool->lock));
//     }

//     // if size if 0 unlock the mutex so other processes can use it
//     if (pool->size == 0 && pool->isShutDown){
//         pthread_mutex_unlock(&(pool->lock));
//         pthread_exit(NULL);
//     }
//     pthread_mutex_unlock(&(pool->lock));
//     // End of while loop
//     que = pool->head;
//     pool->size--;

//     if (pool->size == 0){
//       pool->head = pool->tail = NULL;
//     }
//     else{
//       pool->head = que->next;
//     }

//     if (pool->size == 0 && !pool->isShutDown){
//       // now its empty so we send a signal
//       pthread_cond_signal(&(pool->empty));
//     }

//     pthread_mutex_unlock(&(pool->lock));

//     // Call the function with the arguments
//     (*(que->func)) (que->arg);
//     pool->threads_left++;

//     pthread_mutex_lock(&(pool->lock));
//     free(que);
//     if (pool->threads_left == 1){
//       pthread_mutex_unlock(&(pool->lock));
//       pthread_cond_signal(&pool->empty);
      
//     }
//     else{
//       pthread_mutex_unlock(&(pool->lock));
//     }

//   }
// }


// threadpool create_threadpool(int num_threads_in_pool) {
//   _threadpool *pool;

//   // sanity check the argument
//   if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
//     return NULL;

//   pool = (_threadpool *) malloc(sizeof(_threadpool));
//   if (pool == NULL) {
//     fprintf(stderr, "Error whilst creating threadpool\n");
//     return NULL;
//   }

//   pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads_in_pool);

//   if (!pool->threads){
//     fprintf(stderr, "Error whilst creating threadpool\n");
//     return NULL;
//   }

//   pool->size        = 0;
//   pool->isNotAccept = 0;
//   pool->isShutDown  = 0;
//   pool->head        = NULL;
//   pool->tail        = NULL;
//   pool->num_of_threads = pool->threads_left = num_threads_in_pool;

//   if (pthread_cond_init(&pool->empty, NULL)){
//     fprintf(stderr, "CV initiation error\n");
//     return NULL;
//   }
//   if (pthread_mutex_init(&pool->lock, NULL)){
//     fprintf(stderr, "Mutex error\n");
//     return NULL;
//   }
//   if (pthread_cond_init(&pool->not_empty, NULL)){
//     fprintf(stderr, "CV initiation error\n");
//     return NULL;
//   }

//   for (int i = 0; i < pool->num_of_threads;i++){
//     if (pthread_create(&(pool->threads[i]), NULL, worker_thread, pool)){ // on success returns 0 
//       fprintf(stderr, "Thread couldn't initialize\n");
//       return NULL;
//     }
//   }
//   return (threadpool) pool;
// }


// void dispatch(threadpool from_me, dispatch_fn dispatch_to_here,
// 	      void *arg) {

//   _threadpool *pool = (_threadpool *) from_me;
//   queue* que;
  
//   // take hold of the mutex
//   pthread_mutex_lock(&(pool->lock));

//   que = (queue*)malloc(sizeof(queue));
//   if (que == NULL){
//     fprintf(stderr, "Error in creating the queue in dispatch function\n");
//   }

//   que->func = dispatch_to_here;
//   que->arg = arg;
//   que->next = NULL;

//   if (pool->size == 0){
//     // Initializing queue with one item
//     pool->head = que;
//     pool->tail = que;    
//   }
//   else{
//     // add to the end
//     pool->tail->next = que;
//     pool->tail = que;
//   }

//   // increment the size since we're adding
//   pool->size++;
//   pool->threads_left--;

//   pthread_mutex_unlock(&(pool->lock));
//   // sending a signal that the pool is not empty
//   pthread_cond_signal(&(pool->not_empty));

//   // take hold of the mutex
//   pthread_mutex_lock(&(pool->lock));

//   if (pool->threads_left <= 0){
//     pthread_cond_wait(&(pool->empty), &(pool->lock));
//   }
//   pthread_mutex_unlock(&(pool->lock));
// }

// void destroy_threadpool(threadpool destroyme) {
//   _threadpool *pool = (_threadpool *) destroyme;
//   void* nothing;

//   pthread_mutex_lock(&(pool->lock));
//   pool->isShutDown = 1;
//   pthread_mutex_unlock(&(pool->lock));
//   pthread_cond_broadcast(&(pool->not_empty));  

//   //kill everything.  
//   for(int i = 0;i < pool->num_of_threads;i++) {
//     pthread_cond_broadcast(&(pool->not_empty));
//     pthread_join(pool->threads[i],&nothing);
//   }

//   free(pool->threads);
//   free(pool);

//   // pthread_mutex_destroy(&(pool->lock));
//   // pthread_cond_destroy(&(pool->empty));
//   // pthread_cond_destroy(&(pool->not_empty));

// }

// _threadpool is the internal threadpool structure that is

typedef struct work_st{
	void (*routine) (void*);
	void * arg;
	struct work_st* next;
} work_t;

typedef struct _threadpool_st {
   // you should fill in this structure with whatever you need
	int activethreads;
	int queuesize;			
	pthread_t *threads;	
	work_t* queuehead;	
	work_t* queuetail;		
	pthread_mutex_t lockqueue;	
	pthread_cond_t QueueInUse;
	pthread_cond_t QueueIsEmpty;
	int shutdown;
	int dont_accept;
	int threadsleft;
} _threadpool;

/* This function is the work function of the thread */
void* worker_thread(threadpool p) {
	_threadpool * pool = (_threadpool *) p;
	work_t* cur;	
	int k;

	while(1) {

		pool->queuesize = pool->queuesize;
		pthread_mutex_lock(&(pool->lockqueue));  

		if (pool->queuesize == 0){
			pthread_cond_wait(&(pool->QueueInUse), &(pool->lockqueue));
		}


		while( pool->queuesize == 0) {	  
			if(pool->shutdown) {
				pthread_mutex_unlock(&(pool->lockqueue));
				pthread_exit(NULL);
			}
			

		}

		pthread_mutex_unlock(&(pool->lockqueue));  

		cur = pool->queuehead;	  

		pool->queuesize--;		  
		pool->threadsleft++;

		if(pool->queuesize == 0) {
			pool->queuehead = NULL;
			pool->queuetail = NULL;
		}
		else {
			pool->queuehead = cur->next;
		}

		if(pool->queuesize == 0 && ! pool->shutdown) {
			pthread_cond_signal(&(pool->QueueIsEmpty));
		}
		pthread_mutex_unlock(&(pool->lockqueue));
		(cur->routine) (cur->arg);
		pthread_mutex_lock(&(pool->lockqueue));

		free(cur);
		if (pool->threadsleft == 1){
			pthread_mutex_unlock(&(pool->lockqueue));
			pthread_cond_signal(&(pool->QueueIsEmpty));
		}else{
			pthread_mutex_unlock(&(pool->lockqueue));
		}
	}
}

threadpool create_threadpool(int num_threads_in_pool) {
  _threadpool *pool;

  if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
    return NULL;

  pool = (_threadpool *) malloc(sizeof(_threadpool));
  if (pool == NULL) {
    fprintf(stderr, "Cant create threadpool\n");
    return NULL;
  }

  pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads_in_pool);

  if (!pool->threads){
    fprintf(stderr, "Cant create threadpool\n");
    return NULL;
  }

  pool->queuehead = NULL;
  pool->queuetail = NULL;
  pool->queuesize = 0;
  pool->dont_accept = 0;
  pool->shutdown  = 0;
  pool->activethreads = num_threads_in_pool;
  pool->threadsleft = num_threads_in_pool;

  if (pthread_cond_init(&pool->QueueIsEmpty, NULL)){
    fprintf(stderr, "CV initiation error\n");
    return NULL;
  }
  if (pthread_mutex_init(&pool->lockqueue, NULL)){
    fprintf(stderr, "Mutex error\n");
    return NULL;
  }
  if (pthread_cond_init(&pool->QueueInUse, NULL)){
    fprintf(stderr, "CV initiation error\n");
    return NULL;
  }

  for (int i = 0; i < pool->activethreads;i++){
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
	work_t *cur;
	int k;

	k = pool->queuesize;

	//make a work queue element.  
	cur = (work_t*) malloc(sizeof(work_t));
	if(cur == NULL) {
		fprintf(stderr, "Out of memory creating a work struct!\n");
		return;	
	}

	cur->routine = dispatch_to_here;
	cur->arg = arg;
	cur->next = NULL;

	pthread_mutex_lock(&(pool->lockqueue));

	if(pool->dont_accept) { 
		free(cur); 
		return;
	}
	if(pool->queuesize == 0) {
		pool->queuehead = cur;  
		pool->queuetail = cur;
	} else {
		pool->queuetail->next = cur;
		pool->queuetail = cur;			
	}
	pool->queuesize++;
	pool->threadsleft--;

	pthread_mutex_unlock(&(pool->lockqueue)); 
	pthread_cond_signal(&(pool->QueueInUse)); 
	pthread_mutex_lock(&(pool->lockqueue));

	if (pool->threadsleft == 0){
		pthread_cond_wait(&(pool->QueueIsEmpty), &(pool->lockqueue));
	}

	pthread_mutex_unlock(&(pool->lockqueue));
}

void destroy_threadpool(threadpool destroyme) {
	_threadpool *pool = (_threadpool *) destroyme;
	free(pool->threads);

	pthread_mutex_destroy(&(pool->lockqueue));
	pthread_cond_destroy(&(pool->QueueIsEmpty));
	pthread_cond_destroy(&(pool->QueueInUse));
	return;
}