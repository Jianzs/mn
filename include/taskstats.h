#ifndef TASKSTATS_H
#define TASKSTATS_H

#include <linux/taskstats.h>
#include <stddef.h>
#include <time.h>

struct TaskStatistics {
    int pid;
    int tgid;
    time_t timestamp;
    struct taskstats stats;
};

void print_task_stats(const struct TaskStatistics* stats,
                      int human_readable);

char* task_stats2str(const struct TaskStatistics* stats, char* buf, size_t len);

#endif