#include "utils.h"

uint64_t get_ns_timestamp() {
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    time_t timestamp = current_time.tv_sec*1000000000+current_time.tv_nsec;
    return timestamp;
}

// uint64_t average_ns(uint64_t total, uint64_t count) {
//     if (!count) return 0;
//     return total / count;
// }