#include "utils.h"
#include <unistd.h>

time_t get_ns_timestamp() {
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    time_t timestamp = current_time.tv_sec*1000000000+current_time.tv_nsec;
    return timestamp;
}

void sleep_until(time_t t_target_ns) {
    do {
        time_t t_cur_ns = get_ns_timestamp();
        if (t_target_ns - t_cur_ns < 50 * MICRO_SECOND) break;
        usleep(40);
    } while (1);

    do {
        time_t t_cur_ns = get_ns_timestamp();
        if (t_target_ns <= t_cur_ns) break;
    } while (1);
}