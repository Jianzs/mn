#ifndef UTIL_H
#define UTIL_H

#include <time.h>

#define NANO_SECOND  1
#define MICRO_SECOND 1000
#define MILL_SECOND  1000000

time_t get_ns_timestamp();
void sleep_until(time_t t_target_ns);

#endif