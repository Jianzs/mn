#include <linux/taskstats.h>

struct TaskStatistics {
    int pid;
    int tgid;
    struct taskstats stats;
};

void print_task_stats(const struct TaskStatistics* stats,
                      int human_readable);