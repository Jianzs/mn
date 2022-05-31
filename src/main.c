/*
 * Linux task stats monitor tool. Queries and prints out the kernel's
 * taskstats structure for a given process or thread group id. See
 * https://www.kernel.org/doc/Documentation/accounting/ for more information
 * about the reported fields.
 */
#include <errno.h>
#include <getopt.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/socket.h> 
#include <netlink/handlers.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>

#include "exec.h"
#include "utils.h"
#include "queue.h"
#include "taskstats.h"

#define N_QUERY_THREAD 10

int print_receive_error(struct sockaddr_nl* address,
                        struct nlmsgerr* error, void* arg) {
    fprintf(stderr, "Netlink receive error: %s\n", strerror(-error->error));
    return NL_STOP;
}

void parse_aggregate_task_stats(struct nlattr* attr, int attr_size,
                                struct TaskStatistics* stats) {
    nla_for_each_attr(attr, attr, attr_size, attr_size) {
        switch (attr->nla_type) {
            case TASKSTATS_TYPE_PID:
                stats->pid = nla_get_u32(attr);
                break;
            case TASKSTATS_TYPE_TGID:
                stats->tgid = nla_get_u32(attr);
                break;
            case TASKSTATS_TYPE_STATS:
                nla_memcpy(&stats->stats, attr, sizeof(stats->stats));
                break;
            default:
                break;
        }
    }
}

int parse_task_stats(struct nl_msg* msg, void* arg) {
    time_t t_cur = get_ns_timestamp();
    struct TaskStatistics* stats = (struct TaskStatistics*)malloc(
        sizeof(struct TaskStatistics));
    stats->timestamp = t_cur;

    struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(nlmsg_hdr(msg));
    struct nlattr* attr = genlmsg_attrdata(gnlh, 0);
    int remaining = genlmsg_attrlen(gnlh, 0);
    nla_for_each_attr(attr, attr, remaining, remaining) {
        switch (attr->nla_type) {
            case TASKSTATS_TYPE_AGGR_PID:
            case TASKSTATS_TYPE_AGGR_TGID:
                parse_aggregate_task_stats(nla_data(attr), nla_len(attr),
                                           stats);
                break;
            default:
                break;
        }
    }

    struct ConcurrentQueue* que = (struct ConcurrentQueue*)arg;
    concurrent_queue_push(que, stats);
    return NL_STOP;
}

int send_task_stats_query(struct nl_sock* netlink_socket, int family_id,
                     int command_type, int parameter) {
    struct nl_msg* message = nlmsg_alloc();
    genlmsg_put(message, NL_AUTO_PID, NL_AUTO_SEQ, family_id, 0, NLM_F_REQUEST,
		        TASKSTATS_CMD_GET, TASKSTATS_VERSION);
    nla_put_u32(message, command_type, parameter);
    int result = nl_send(netlink_socket, message);
    nlmsg_free(message);
    return result < 0;
}

struct ProcessThreadArgs {
    struct ConcurrentQueue *que;
    FILE *file;
    int human_readable;
};

void * process_task_stats(void *arg) {
    struct ProcessThreadArgs *args = (struct ProcessThreadArgs*)arg;
    struct TaskStatistics *stats;
    while ((stats = concurrent_queue_pop(args->que)) != NULL) {
        if (args->file == NULL) {
            print_task_stats(stats, args->human_readable);
        } else {
            char buf[200];
            task_stats2str(stats, buf, 200);
            fprintf(args->file, "%llu\t%s\n", get_ns_timestamp(), buf);
        }
        free(stats);
    }
    pthread_exit(NULL);
}

struct QueryThreadArgs {
    struct nl_sock* netlink_socket;
    int family_id;
    int command_type;
    int pid;
    struct nl_cb* callbacks;
};

void signal_handler(int sig) {
    printf("Caught signal %d\n", sig);
}
void * query_task_stats(void *arg) {
    printf("++ query thread Hello\n");
    signal(SIGUSR1, signal_handler);
    printf("-- query thread Hello\n");
    struct QueryThreadArgs *args = (struct QueryThreadArgs*)arg;

    // sigset_t set;
    // sigemptyset(&set);                                                             
    // if(sigaddset(&set, SIGUSR1) == -1) {                                           
    //     perror("Sigaddset error");                                                  
    //     pthread_exit((void *)1);
    // }

    while (1) {
        printf("~ before pause\n");
        pause();
        printf("@ after pause\n");
        // int sig;
        // int ret = sigwait(&set, &sig);
        // if(ret || sig != SIGUSR1) {                                                 
        //     perror("Sigwait error");                                                    
        //     pthread_exit((void *)2);                                                    
        // }

        int ret = send_task_stats_query(args->netlink_socket, args->family_id, 
                                        args->command_type, args->pid);
        if (ret) {
            perror("Failed to query taskstats");
            continue;
        }

        ret = nl_recvmsgs(args->netlink_socket, args->callbacks);
        if (ret) {
            perror("Failed to receive message\n");
            continue;
        }
    }
}

void print_usage() {
  printf("Linux task stats monitor tool\n"
         "\n"
         "Usage: mn [options] [-- custom command]\n"
         "\n"
         "Options:\n"
         "  --help           Print this usage\n"
         "  --pid PID        Print stats for the process id PID, ignore when "
         "custom command is not empty\n"
         "  --tgid TGID      Print stats for the thread group id TGID\n"
         "  --period MS      Set the query period in millsecond, default 1000ms\n"
         "  --raw            Print raw numbers instead of human readable units\n"
         "  --out FILE       Write the record to the FILE, order of the columns "
         "is the same as in the URL below\n"
         "  --cmd-out FILE   Redict custom command stdout and stderr to the FILE\n"
         "\n"
         "Either PID or TGID or CUSTOM COMMAND must be specified. For more "
         "documentation about the reported fields, see\n"
         "https://www.kernel.org/doc/Documentation/accounting/"
         "taskstats-struct.txt\n");
}

int main(int argc, char** argv) {
    /* parse command line parameters */
    int command_type = 0;
    int pid = 0;
    int human_readable = 1;
    FILE *out_file = NULL;
    int custom_cmd_len = 0;
    char **custom_cmd_arg = NULL;
    char *custom_cmd_out = NULL;
    int period = 1000 * MILL_SECOND;

    const struct option long_options[] = {
        {"help", no_argument, 0, 0},
        {"pid", required_argument, 0, 0},
        {"tgid", required_argument, 0, 0},
        {"raw", no_argument, 0, 0},
        {"out", required_argument, 0, 0},
        {"cmd-out", required_argument, 0, 0},
        {"period", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    while (1) {
        int option_index = 0;
        int option_char = getopt_long_only(argc, argv, "", long_options,
                                           &option_index);
        if (option_char == -1) {
            break;
        }
        switch (option_index) {
            case 0:
                print_usage();
                return EXIT_SUCCESS;
            case 1:
                command_type = TASKSTATS_CMD_ATTR_PID;
                pid = atoi(optarg);
                break;
            case 2:
                command_type = TASKSTATS_CMD_ATTR_TGID;
                pid = atoi(optarg);
                break;
            case 3:
                human_readable = 0;
                break;
            case 4:
                out_file = fopen(optarg, "w");
                if(out_file == NULL){
                    fprintf(stderr, "Unable open the out file\n");   
                    return EXIT_FAILURE;      
                }
            case 5:
                custom_cmd_out = optarg;
                break;
            case 6:
                period = atoi(optarg) * MILL_SECOND;
                break;
            default:
                break;
        };
    }
    custom_cmd_len = argc - optind;
    custom_cmd_arg = argv + optind;
    if (custom_cmd_len) {
        command_type = TASKSTATS_CMD_ATTR_PID;
    }
    if (!pid && !custom_cmd_len) {
        fprintf(stderr, "Either PID or TGID or CUSTOM COMMAND must be specified\n");
        return EXIT_FAILURE;
    }

    /* generate netlink connection */
    struct nl_sock* netlink_socket = nl_socket_alloc();
    if (!netlink_socket) {
        fprintf(stderr, "Unable to allocate netlink socket\n");
        goto error;
    }
    int ret = genl_connect(netlink_socket);
    if (ret < 0) {
        nl_perror(ret, "Unable to open netlink socket (are you root?)");
        goto error;
    }
    int family_id = genl_ctrl_resolve(netlink_socket, TASKSTATS_GENL_NAME);
    if (family_id < 0) {
        nl_perror(family_id, "Unable to determine taskstats family id "
                  "(does your kernel support taskstats?)");
        goto error;
    }

    /* used for communicating between master thread and taskstats thread */
    struct ConcurrentQueue que;
    concurrent_queue_init(&que);
    
    /* create thread for processing task stats */
    struct ProcessThreadArgs process_args = {
        .que = &que,
        .file = out_file,
        .human_readable = human_readable
    };
    pthread_t process_task_stats_thread;
    ret = pthread_create(&process_task_stats_thread, NULL, &process_task_stats, 
                         (void *)(&process_args));
    if (ret) {
        fprintf(stderr, "Unable to create thread, %d\n", ret);
        goto error;
    }

    /* register callback */
    struct nl_cb* callbacks = nl_cb_get(nl_cb_alloc(NL_CB_CUSTOM));
    nl_cb_set(callbacks, NL_CB_MSG_IN, NL_CB_CUSTOM, &parse_task_stats, &que);
    nl_cb_err(callbacks, NL_CB_CUSTOM, &print_receive_error, &family_id);
    
    /* create thread for send task stats query */
    struct QueryThreadArgs query_args = {
        .netlink_socket = netlink_socket,
        .family_id = family_id,
        .command_type = command_type,
        .pid = pid,
        .callbacks = callbacks
    };
    pthread_t query_task_stats_threads[N_QUERY_THREAD];
    for (int i = 0; i < N_QUERY_THREAD; i++) {
        ret = pthread_create(&query_task_stats_threads[i], NULL, &query_task_stats, 
                             (void *)(&query_args));
        if (ret) {
            fprintf(stderr, "Unable to create thread, %d\n", ret);
            goto error;
        }
    }
    
    sleep(1);

    /* run custom command */
    if (custom_cmd_len) {
        signal(SIGCHLD, SIG_IGN); // avoid <defunct> child process 
        pid = exec_command(custom_cmd_len, custom_cmd_arg, custom_cmd_out);
    }

    

    /* monitor the target process */
    unsigned long long kill_total, send_total, recv_total;
    unsigned long long kill_max, send_max, recv_max;
    unsigned cnt = 0;
    kill_total = send_total = recv_total = 0;
    kill_max = send_max = recv_max = 0;
    int query_thread_idx = 0;
    time_t t_next_iter = get_ns_timestamp();
    do {
        time_t ts_b_kill = get_ns_timestamp();
        int killed = kill(pid, 0); // after being killed, query the last time
        time_t ts_a_kill = get_ns_timestamp();

        // printf("** kill\n");
        ret = pthread_kill(query_task_stats_threads[query_thread_idx], SIGUSR1);
        if (ret) {
            fprintf(stderr, "Kill query thread failed, %d\n", ret);
            goto error;
        }
        query_thread_idx = (query_thread_idx+1) % N_QUERY_THREAD;

        
        // ret = send_task_stats_query(netlink_socket, family_id, command_type, pid);
        // if (ret) {
        //     nl_perror(ret, "Failed to query taskstats");
        //     goto error;
        // }
        time_t ts_a_send = get_ns_timestamp();
        // ret = nl_recvmsgs(netlink_socket, callbacks);
        // if (ret) {
        //     nl_perror(ret, "Failed to receive message");
        //     goto error;
        // }
        // time_t ts_a_recv = get_ns_timestamp();
        if (killed) {
            break;
        }
        kill_total += ts_a_kill - ts_b_kill;
        send_total += ts_a_send - ts_a_kill;
        // recv_total += ts_a_recv - ts_a_send;
#define max(a, b) (a > b ? a : b)
        kill_max = max(kill_max, ts_a_kill - ts_b_kill);
        send_max = max(send_max, ts_a_send - ts_a_kill);
        // recv_max = max(recv_max, ts_a_recv - ts_a_send);
        cnt ++;
        t_next_iter += period;
        sleep_until(t_next_iter);
    } while (1);
    concurrent_queue_push(&que, NULL);
    
    printf("%lf %lf %lf\n", kill_total*1./cnt, send_total*1./cnt, recv_total*1./cnt);
    printf("%llu %llu %llu\n", kill_max, send_max, recv_max);

    pthread_join(query_task_stats_threads[0], NULL);

    nl_cb_put(callbacks);
    nl_socket_free(netlink_socket);
    return EXIT_SUCCESS;

error:
    if (netlink_socket) {
        nl_socket_free(netlink_socket);
    }
    return EXIT_FAILURE;
}