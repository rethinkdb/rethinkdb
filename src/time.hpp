#ifndef TIME_HPP_
#define TIME_HPP_

#include <stdint.h>
#include <time.h>

// Monotonic timer.  USE THIS!
timespec clock_monotonic();

// get_ticks() is a wrapper around clock_monotonic() which returns a straight-up 64-bit
// nanosecond counter.
struct ticks_t {
    int64_t nanos;
};
ticks_t get_ticks();

// get_kiloticks() is get_ticks() / 1000.  Used in migrating from legacy wall-clock
// current_microtime().
struct kiloticks_t {
    int64_t micros;
};
kiloticks_t get_kiloticks();

// Real-time wallclock timer.  Non-monotonic, could step backwards or forwards.  Don't
// use this, unless you want to use this.
timespec clock_realtime();

void add_to_timespec(timespec *ts, int32_t nanoseconds);
timespec subtract_timespecs(const timespec &t1, const timespec &t2);
bool operator<(const struct timespec &t1, const struct timespec &t2);
bool operator>(const struct timespec &t1, const struct timespec &t2);
bool operator<=(const struct timespec &t1, const struct timespec &t2);
bool operator>=(const struct timespec &t1, const struct timespec &t2);

ticks_t secs_to_ticks(time_t secs);
double ticks_to_secs(ticks_t ticks);

// Wall-clock time in seconds.
time_t get_realtime_secs();

// Wall-clock time in microseconds.
typedef uint64_t microtime_t;
microtime_t current_microtime();


#endif  // TIME_HPP_
