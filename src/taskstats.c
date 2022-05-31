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

char* task_stats2str(const struct TaskStatistics* stats, char* buf, size_t len) {
    char * const buf_orig = buf;
    const struct taskstats* s = &stats->stats;
    int l;
#define snprintllu(d) do{l = snprintf(buf, len, "%llu\t", d); buf += l; len -= l;}while(0)
    /* 1) Common and basic accounting fields: */
	/* The version number of this struct. This field is always set to
	 * TAKSTATS_VERSION, which is defined in <linux/taskstats.h>.
	 * Each time the struct is changed, the value should be incremented.
	 */
	// snprintllu(s->version);

  	/* The exit code of a task. */
	// snprintllu(s->ac_exitcode);		/* Exit status */

  	/* The accounting flags of a task as defined in <linux/acct.h>
	 * Defined values are AFORK, ASU, ACOMPAT, ACORE, and AXSIG.
	 */
	// snprintllu(s->ac_flag);		/* Record flags */

  	/* The value of task_nice() of a task. */
	// snprintllu(s->ac_nice);		/* task_nice */

  	/* The name of the command that started this task. */
	// char	ac_comm[TS_COMM_LEN];	/* Command name */
    l = snprintf(buf, len, "%s\t", s->ac_comm);
    buf += l;
    len -= l;

  	/* The scheduling discipline as set in task->policy field. */
	// snprintllu(s->ac_sched);		/* Scheduling discipline */

	// snprintllu(s->ac_pad[3]);
	// snprintllu(s->ac_uid);			/* User ID */
	// snprintllu(s->ac_gid);			/* Group ID */
	// snprintllu(s->ac_pid);			/* Process ID */
	// snprintllu(s->ac_ppid);		/* Parent process ID */

  	/* The time when a task begins, in [secs] since 1970. */
	snprintllu(s->ac_btime);		/* Begin time [sec since 1970] */

  	/* The elapsed time of a task, in [usec]. */
	snprintllu(s->ac_etime);		/* Elapsed time [usec] */

  	/* The user CPU time of a task, in [usec]. */
	snprintllu(s->ac_utime);		/* User CPU time [usec] */

  	/* The system CPU time of a task, in [usec]. */
	snprintllu(s->ac_stime);		/* System CPU time [usec] */

  	/* The minor page fault count of a task, as set in task->min_flt. */
	snprintllu(s->ac_minflt);		/* Minor Page Fault Count */

	/* The major page fault count of a task, as set in task->maj_flt. */
	snprintllu(s->ac_majflt);		/* Major Page Fault Count */

    /* 2) Delay accounting fields: */
	/* Delay accounting fields start
	 *
	 * All values, until the comment "Delay accounting fields end" are
	 * available only if delay accounting is enabled, even though the last
	 * few fields are not delays
	 *
	 * xxx_count is the number of delay values recorded
	 * xxx_delay_total is the corresponding cumulative delay in nanoseconds
	 *
	 * xxx_delay_total wraps around to zero on overflow
	 * xxx_count incremented regardless of overflow
	 */

	/* Delay waiting for cpu, while runnable
	 * count, delay_total NOT updated atomically
	 */
	snprintllu(s->cpu_count);
	snprintllu(s->cpu_delay_total);

	/* Following four fields atomically updated using task->delays->lock */

	/* Delay waiting for synchronous block I/O to complete
	 * does not account for delays in I/O submission
	 */
	snprintllu(s->blkio_count);
	snprintllu(s->blkio_delay_total);

	/* Delay waiting for page fault I/O (swap in only) */
	snprintllu(s->swapin_count);
	snprintllu(s->swapin_delay_total);

	/* cpu "wall-clock" running time
	 * On some architectures, value will adjust for cpu time stolen
	 * from the kernel in involuntary waits due to virtualization.
	 * Value is cumulative, in nanoseconds, without a corresponding count
	 * and wraps around to zero silently on overflow
	 */
	snprintllu(s->cpu_run_real_total);

	/* cpu "virtual" running time
	 * Uses time intervals seen by the kernel i.e. no adjustment
	 * for kernel's involuntary waits due to virtualization.
	 * Value is cumulative, in nanoseconds, without a corresponding count
	 * and wraps around to zero silently on overflow
	 */
	snprintllu(s->cpu_run_virtual_total);
	/* Delay accounting fields end */
	/* version 1 ends here */

    /* 3) Extended accounting fields */
	/* Extended accounting fields start */

	/* Accumulated RSS usage in duration of a task, in MBytes-usecs.
	 * The current rss usage is added to this counter every time
	 * a tick is charged to a task's system time. So, at the end we
	 * will have memory usage multiplied by system time. Thus an
	 * average usage per system time unit can be calculated.
	 */
	snprintllu(s->coremem);		/* accumulated RSS usage in MB-usec */

  	/* Accumulated virtual memory usage in duration of a task.
	 * Same as acct_rss_mem1 above except that we keep track of VM usage.
	 */
	snprintllu(s->virtmem);		/* accumulated VM usage in MB-usec */

  	/* High watermark of RSS usage in duration of a task, in KBytes. */
	snprintllu(s->hiwater_rss);		/* High-watermark of RSS usage */

  	/* High watermark of VM  usage in duration of a task, in KBytes. */
	snprintllu(s->hiwater_vm);		/* High-water virtual memory usage */

	/* The following four fields are I/O statistics of a task. */
	snprintllu(s->read_char);		/* bytes read */
	snprintllu(s->write_char);		/* bytes written */
	snprintllu(s->read_syscalls);		/* read syscalls */
	snprintllu(s->write_syscalls);		/* write syscalls */

	/* Extended accounting fields end */

    /* 4) Per-task and per-thread statistics */
	snprintllu(s->nvcsw);			/* Context voluntary switch counter */
	snprintllu(s->nivcsw);			/* Context involuntary switch counter */

    /* 5) Time accounting for SMT machines */
	snprintllu(s->ac_utimescaled);		/* utime scaled on frequency etc */
	snprintllu(s->ac_stimescaled);		/* stime scaled on frequency etc */
	snprintllu(s->cpu_scaled_run_real_total); /* scaled cpu_run_real_total */

    /* 6) Extended delay accounting fields for memory reclaim
	      Delay waiting for memory reclaim */
	snprintllu(s->freepages_count);
	snprintllu(s->freepages_delay_total);
#undef snprintllu
    return buf_orig;
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