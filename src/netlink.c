#include "netlink.h"

int send_command(struct nl_sock* netlink_socket, uint16_t nlmsg_type,
                 uint32_t nlmsg_pid, uint8_t genl_cmd, uint16_t nla_type,
                 void* nla_data, int nla_len) {
    struct nl_msg* message = nlmsg_alloc();
    int seq = 0;
    int version = 1;
    int header_length = 0;
    int flags = NLM_F_REQUEST;
    genlmsg_put(message, nlmsg_pid, seq, nlmsg_type, header_length, flags,
                genl_cmd, version);
    nla_put(message, nla_type, nla_len, nla_data);
    /* Override the header flags since we don't want NLM_F_ACK. */
    struct nlmsghdr* header = nlmsg_hdr(message);
    header->nlmsg_flags = flags;
    int result = nl_send(netlink_socket, message);
    nlmsg_free(message);
    return result;
}

int print_receive_error(struct sockaddr_nl* address __unused,
                        struct nlmsgerr* error, void* arg __unused) {
    fprintf(stderr, "Netlink receive error: %s\n", strerror(-error->error));
    return NL_STOP;
}

int parse_family_id(struct nl_msg* msg, void* arg) {
    struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(nlmsg_hdr(msg));
    struct nlattr* attr = genlmsg_attrdata(gnlh, 0);
    int remaining = genlmsg_attrlen(gnlh, 0);
    do {
        if (attr->nla_type == CTRL_ATTR_FAMILY_ID) {
            *((int*)arg) = nla_get_u16(attr);
            return NL_STOP;
        }
    } while ((attr = nla_next(attr, &remaining)));
    return NL_OK;
}

int get_family_id(struct nl_sock* netlink_socket, const char* name) {
    if (send_command(netlink_socket, GENL_ID_CTRL, getpid(),
                     CTRL_CMD_GETFAMILY,
                     CTRL_ATTR_FAMILY_NAME,
                     (void*)name, strlen(name) + 1) < 0) {
        return 0;
    }
    int family_id = 0;
    struct nl_cb* callbacks = nl_cb_get(nl_cb_alloc(NL_CB_VALID));
    nl_cb_set(callbacks, NL_CB_VALID, NL_CB_DEFAULT, &parse_family_id,
              &family_id);
    nl_cb_err(callbacks, NL_CB_DEFAULT, &print_receive_error, NULL);
    if (nl_recvmsgs(netlink_socket, callbacks) < 0) {
        return 0;
    }
    nl_cb_put(callbacks);
    return family_id;
}

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