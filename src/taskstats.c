#include "taskstats.h"

#include <stdio.h>
#include <time.h>
#include "utils.h"

double average_ms(unsigned long long total, unsigned long long count);
unsigned long long average_ns(unsigned long long total,
                              unsigned long long count);

void print_task_stats(const struct TaskStatistics* stats,
                      int human_readable) {
    const struct taskstats* s = &stats->stats;
    time_t cur_time = get_ns_timestamp();
    int prefix_sec = cur_time / 1000000000;
    int ms = (cur_time % 1000000000) / 1000000;
    int us = (cur_time % 1000000) / 1000;
    int ns = cur_time % 1000;
    printf("\n\n=========== (%d) %d ms %d us %d ns ===========\n", prefix_sec, ms, us, ns);
    printf("\nBasic task statistics\n");
    printf("---------------------\n");
    printf("%-25s%d\n", "Stats version:", s->version);
    printf("%-25s%d\n", "Exit code:", s->ac_exitcode);
    printf("%-25s0x%x\n", "Flags:", s->ac_flag);
    printf("%-25s%d\n", "Nice value:", s->ac_nice);
    printf("%-25s%s\n", "Command name:", s->ac_comm);
    printf("%-25s%d\n", "Scheduling discipline:", s->ac_sched);
    printf("%-25s%d\n", "UID:", s->ac_uid);
    printf("%-25s%d\n", "GID:", s->ac_gid);
    printf("%-25s%d\n", "PID:", s->ac_pid);
    printf("%-25s%d\n", "PPID:", s->ac_ppid);
    if (human_readable) {
        time_t begin_time = s->ac_btime;
        printf("%-25s%s", "Begin time:", ctime(&begin_time));
    } else {
        printf("%-25s%d sec\n", "Begin time:", s->ac_btime);
    }
    printf("%-25s%llu usec\n", "Elapsed time:", s->ac_etime);
    printf("%-25s%llu usec\n", "User CPU time:", s->ac_utime);
    printf("%-25s%llu usec\n", "System CPU time:", s->ac_stime);
    printf("%-25s%llu\n", "Minor page faults:", s->ac_minflt);
    printf("%-25s%llu\n", "Major page faults:", s->ac_majflt);
    printf("%-25s%llu usec\n", "Scaled user time:", s->ac_utimescaled);
    printf("%-25s%llu usec\n", "Scaled system time:", s->ac_stimescaled);
    printf("\nDelay accounting\n");
    printf("----------------\n");
    printf("       %15s%15s%15s%15s%15s%15s\n",
           "Count",
           human_readable ? "Delay (ms)" : "Delay (ns)",
           "Average delay",
           "Real delay",
           "Scaled real",
           "Virtual delay");
    if (!human_readable) {
        printf("CPU    %15llu%15llu%15llu%15llu%15llu%15llu\n",
               s->cpu_count,
               s->cpu_delay_total,
               average_ns(s->cpu_delay_total, s->cpu_count),
               s->cpu_run_real_total,
               s->cpu_scaled_run_real_total,
               s->cpu_run_virtual_total);
        printf("IO     %15llu%15llu%15llu\n",
               s->blkio_count,
               s->blkio_delay_total,
               average_ns(s->blkio_delay_total, s->blkio_count));
        printf("Swap   %15llu%15llu%15llu\n",
               s->swapin_count,
               s->swapin_delay_total,
               average_ns(s->swapin_delay_total, s->swapin_count));
        printf("Reclaim%15llu%15llu%15llu\n",
               s->freepages_count,
               s->freepages_delay_total,
               average_ns(s->freepages_delay_total, s->freepages_count));
    } else {
        const double ms_per_ns = 1e6;
        printf("CPU    %15llu%15.3f%15.3f%15.3f%15.3f%15.3f\n",
               s->cpu_count,
               s->cpu_delay_total / ms_per_ns,
               average_ms(s->cpu_delay_total, s->cpu_count),
               s->cpu_run_real_total / ms_per_ns,
               s->cpu_scaled_run_real_total / ms_per_ns,
               s->cpu_run_virtual_total / ms_per_ns);
        printf("IO     %15llu%15.3f%15.3f\n",
               s->blkio_count,
               s->blkio_delay_total / ms_per_ns,
               average_ms(s->blkio_delay_total, s->blkio_count));
        printf("Swap   %15llu%15.3f%15.3f\n",
               s->swapin_count,
               s->swapin_delay_total / ms_per_ns,
               average_ms(s->swapin_delay_total, s->swapin_count));
        printf("Reclaim%15llu%15.3f%15.3f\n",
               s->freepages_count,
               s->freepages_delay_total / ms_per_ns,
               average_ms(s->freepages_delay_total, s->freepages_count));
    }
    printf("\nExtended accounting fields\n");
    printf("--------------------------\n");
    if (human_readable && s->ac_stime) {
        printf("%-25s%.3f MB\n", "Average RSS usage:",
               (double)s->coremem / s->ac_stime);
        printf("%-25s%.3f MB\n", "Average VM usage:",
               (double)s->virtmem / s->ac_stime);
    } else {
        printf("%-25s%llu MB\n", "Accumulated RSS usage:", s->coremem);
        printf("%-25s%llu MB\n", "Accumulated VM usage:", s->virtmem);
    }
    printf("%-25s%llu KB\n", "RSS high water mark:", s->hiwater_rss);
    printf("%-25s%llu KB\n", "VM high water mark:", s->hiwater_vm);
    printf("%-25s%llu\n", "IO bytes read:", s->read_char);
    printf("%-25s%llu\n", "IO bytes written:", s->write_char);
    printf("%-25s%llu\n", "IO read syscalls:", s->read_syscalls);
    printf("%-25s%llu\n", "IO write syscalls:", s->write_syscalls);
    printf("\nPer-task/thread statistics\n");
    printf("--------------------------\n");
    printf("%-25s%llu\n", "Voluntary switches:", s->nvcsw);
    printf("%-25s%llu\n", "Involuntary switches:", s->nivcsw);
#if TASKSTATS_VERSION > 8
    if (s->version > 8) {
        printf("%-25s%llu\n", "Thrashing count:", s->thrashing_count);
        printf("%-25s%llu\n", "Thrashing delay total:", s->thrashing_delay_total);
    }
#endif
}

/* utility function */
double average_ms(unsigned long long total, unsigned long long count) {
    if (!count) {
        return 0;
    }
    return ((double)total) / count / 1e6;
}

unsigned long long average_ns(unsigned long long total,
                              unsigned long long count) {
    if (!count) {
        return 0;
    }
    return total / count;
}