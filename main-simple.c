#include <stdlib.h>
#include <linux/taskstats.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "common.h"
#include "utils.h"
#include "exec.h"

void print_usage();
int msg_recv_cb(struct nl_msg *, void *);

int query_task_stats(struct nl_sock* netlink_socket, int family_id,
                     int command_type, int parameter,
                     struct TaskStatistics* stats) {
    memset(stats, 0, sizeof(*stats));
    int result = send_command(netlink_socket, family_id, getpid(),
                              TASKSTATS_CMD_GET, command_type, &parameter,
                              sizeof(parameter));
    if (result < 0) {
        return result;
    }
    struct nl_cb* callbacks = nl_cb_get(nl_cb_alloc(NL_CB_VALID));
    nl_cb_set(callbacks, NL_CB_VALID, NL_CB_DEFAULT, &parse_task_stats, stats);
    nl_cb_err(callbacks, NL_CB_DEFAULT, &print_receive_error, &family_id);
    result = nl_recvmsgs(netlink_socket, callbacks);
    if (result < 0) {
        return result;
    }
    nl_cb_put(callbacks);
    return stats->pid || stats->tgid;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        exit(1);
    }

    struct nl_handle *sock = nl_handle_alloc();
    if(!sock){
		nl_perror("nl_handle_alloc");
		exit(1);
    }
    // Connect to generic netlink socket on kernel side
    if(genl_connect(sock) < 0){
		nl_perror("genl_connect");
  		exit(1);	
    }
    DEBUG_PRINT("get sock successfully\n");

    // get the id for the TASKSTATS generic family
    int family_id = get_family_id(sock, TASKSTATS_GENL_NAME);
    if (!family_id) {
        perror("Unable to determine taskstats family id "
               "(does your kernel support taskstats?)");
        goto error;
    }
    DEBUG_PRINT("family end\n");

    // register for task exit events
    struct nl_msg *msg = nlmsg_alloc();

    genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family_id, 0, NLM_F_REQUEST,
            TASKSTATS_CMD_GET, TASKSTATS_VERSION);
    
    int pid = exec_command(argc - 1, argv + 1);
    nla_put_u32(msg, TASKSTATS_CMD_ATTR_PID, pid);

    nl_send_auto_complete(sock, msg);
    nlmsg_free(msg);

    // specify a callback for inbound messages
    nl_socket_modify_cb(sock, NL_CB_MSG_IN, NL_CB_CUSTOM, msg_recv_cb, NULL);
    if (pid > 0) {
        if(nl_recvmsgs_default(sock) < 0){
			nl_perror("nl_recvmsgs_default");
			exit(-1);
		}
    } else {
        while (1)
            nl_recvmsgs_default(sock);
    }
    return 0;
}


void print_usage() {
  printf("Linux task stats reporting tool\n"
         "\n"
         "Usage: taskstats [options]\n"
         "\n"
         "Options:\n"
         "  --help        This text\n"
         "  --pid PID     Print stats for the process id PID\n"
         "  --tgid TGID   Print stats for the thread group id TGID\n"
         "  --raw         Print raw numbers instead of human readable units\n"
         "\n"
         "Either PID or TGID must be specified. For more documentation about "
         "the reported fields, see\n"
         "https://www.kernel.org/doc/Documentation/accounting/"
         "taskstats-struct.txt\n");
}

#define printllu(str, value)    printf("%s: %llu\n", str, value)

int msg_recv_cb(struct nl_msg *nlmsg, void *arg)
{
    struct nlmsghdr *nlhdr;
    struct nlattr *nlattrs[TASKSTATS_TYPE_MAX + 1];
    struct nlattr *nlattr;
    struct taskstats *stats;
    int rem;

    nlhdr = nlmsg_hdr(nlmsg);

    // validate message and parse attributes
    genlmsg_parse(nlhdr, 0, nlattrs, TASKSTATS_TYPE_MAX, 0);
    if (nlattr = nlattrs[TASKSTATS_TYPE_AGGR_PID]) {
        stats = nla_data(nla_next(nla_data(nlattr), &rem));

        printf("---\n");
        printf("pid: %u\n", stats->ac_pid);
        printf("command: %s\n", stats->ac_comm);
        printf("status: %u\n", stats->ac_exitcode);
        printf("time:\n");
        printf("  start: %u\n", stats->ac_btime);

        printllu("  elapsed", stats->ac_etime);
        printllu("  user", stats->ac_utime);
        printllu("  system", stats->ac_stime);
        printf("memory:\n");
        printf("  bytetime:\n");
        printllu("    rss", stats->coremem);
        printllu("    vsz", stats->virtmem);
        printf("  peak:\n");
        printllu("    rss", stats->hiwater_rss);
        printllu("    vsz", stats->hiwater_vm);
        printf("io:\n");
        printf("  bytes:\n");
        printllu("    read", stats->read_char);
        printllu("    write", stats->write_char);
        printf("  syscalls:\n");
        printllu("    read", stats->read_syscalls);
        printllu("    write", stats->write_syscalls);

        printf("CPU    %15llu%15llu%15llu%15llu%15llu%15llu\n",
               stats->cpu_count,
               stats->cpu_delay_total,
               average_ns(stats->cpu_delay_total, stats->cpu_count),
               stats->cpu_run_real_total,
               stats->cpu_scaled_run_real_total,
               stats->cpu_run_virtual_total);
        printf("IO     %15llu%15llu%15llu\n",
               stats->blkio_count,
               stats->blkio_delay_total,
               average_ns(stats->blkio_delay_total, stats->blkio_count));
        printf("Swap   %15llu%15llu%15llu\n",
               stats->swapin_count,
               stats->swapin_delay_total,
               average_ns(stats->swapin_delay_total, stats->swapin_count));
        printf("Reclaim%15llu%15llu%15llu\n",
               stats->freepages_count,
               stats->freepages_delay_total,
               average_ns(stats->freepages_delay_total, stats->freepages_count));
    }
    return 0;
}