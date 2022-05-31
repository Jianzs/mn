#include "queue.h"
#include <stdlib.h>
#define SIZE 1000
#define NEXT_IDX(i) ((i+1)%SIZE)

void concurrent_queue_init(struct ConcurrentQueue * const q) {
    q->taskstats = (struct TaskStatistics**)malloc(
        SIZE * sizeof(struct TaskStatistics*));
    q->front = 0;
    q->tail = 0;
}

void concurrent_queue_push(struct ConcurrentQueue * const q, 
                           struct TaskStatistics* taskstat) {
    pthread_mutex_lock(&q->lock);
    if (NEXT_IDX(q->tail) == q->front) {
        pthread_cond_wait(&q->space_cond, &q->lock);
    }
    q->taskstats[q->tail] = taskstat;
    q->tail = NEXT_IDX(q->tail);
    pthread_cond_broadcast(&q->data_cond);
    pthread_mutex_unlock(&q->lock);
}

struct TaskStatistics* concurrent_queue_pop(struct ConcurrentQueue * const q) {
    pthread_mutex_lock(&q->lock);
    if (q->front == q->tail) {
        pthread_cond_wait(&q->data_cond, &q->lock);
    }
    struct TaskStatistics* taskstat = q->taskstats[q->front];
    q->front = NEXT_IDX(q->front);
    pthread_cond_broadcast(&q->space_cond);
    pthread_mutex_unlock(&q->lock);
    return taskstat;
}