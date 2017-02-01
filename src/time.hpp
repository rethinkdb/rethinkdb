#ifndef TIME_HPP_
#define TIME_HPP_

#include <stdint.h>
#include <time.h>

typedef uint64_t microtime_t;

microtime_t current_microtime();

timespec clock_monotonic();
timespec clock_realtime();

void add_to_timespec(timespec *ts, int32_t nanoseconds);
timespec subtract_timespecs(const timespec &t1, const timespec &t2);
bool operator<(const struct timespec &t1, const struct timespec &t2);
bool operator>(const struct timespec &t1, const struct timespec &t2);
bool operator<=(const struct timespec &t1, const struct timespec &t2);
bool operator>=(const struct timespec &t1, const struct timespec &t2);

struct ticks_t {
    int64_t nanos;
};
ticks_t secs_to_ticks(time_t secs);
ticks_t get_ticks();
double ticks_to_secs(ticks_t ticks);

time_t get_realtime_secs();

#endif  // TIME_HPP_
