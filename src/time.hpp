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

typedef uint64_t ticks_t;
ticks_t secs_to_ticks(time_t secs);
ticks_t get_ticks();
time_t get_secs();
double ticks_to_secs(ticks_t ticks);


#endif  // TIME_HPP_
