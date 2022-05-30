#include "utils.h"

time_t get_ns_timestamp() {
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    time_t timestamp = current_time.tv_sec*1000000000+current_time.tv_nsec;
    return timestamp;
}