#ifndef QUEUE_H
#define QUEUE_H

#include "taskstats.h"
#include <pthread.h>

struct ConcurrentQueue {
    struct TaskStatistics **taskstats;
    int front, tail;
    pthread_mutex_t lock;
    pthread_cond_t space_cond;
    pthread_cond_t data_cond;
};


void concurrent_queue_init(struct ConcurrentQueue * const q);
void concurrent_queue_push(struct ConcurrentQueue * const q, 
                           struct TaskStatistics* taskstat);
struct TaskStatistics* concurrent_queue_pop(struct ConcurrentQueue * const q);

#endif