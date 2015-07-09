#include "time.hpp"

#include <sys/time.h>

#ifdef __MACH__
#include <mach/mach_time.h>
#include "thread_local.hpp"
#include "utils.hpp"
#endif

#include "config/args.hpp"
#include "errors.hpp"

microtime_t current_microtime() {
    // This could be done more efficiently, surely.
    struct timeval t;
    DEBUG_VAR int res = gettimeofday(&t, NULL);
    rassert(0 == res);
    return uint64_t(t.tv_sec) * MILLION + t.tv_usec;
}

ticks_t secs_to_ticks(time_t secs) {
    return static_cast<ticks_t>(secs) * BILLION;
}

#ifdef __MACH__
TLS(mach_timebase_info_data_t, mach_time_info);
#endif  // __MACH__

timespec clock_monotonic() {
#ifdef __MACH__
    mach_timebase_info_data_t mach_time_info = TLS_get_mach_time_info();
    if (mach_time_info.denom == 0) {
        mach_timebase_info(&mach_time_info);
        guarantee(mach_time_info.denom != 0);
        TLS_set_mach_time_info(mach_time_info);
    }
    const uint64_t t = mach_absolute_time();
    uint64_t nanosecs = t * mach_time_info.numer / mach_time_info.denom;
    timespec ret;
    ret.tv_sec = nanosecs / BILLION;
    ret.tv_nsec = nanosecs % BILLION;
    return ret;
#else
#ifdef __sun
    /*
     * Ideally we'd use CLOCK_MONOTONIC here, but its use is restricted by
     * default, notably inside zones.
     */
    timespec ret;
    hrtime_t hrt = gethrtime();
    ret.tv_sec = hrt / BILLION;
    ret.tv_nsec = hrt % BILLION;
#else
    int res = clock_gettime(CLOCK_MONOTONIC, &ret);
    guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTIC, ...) failed");
#endif
    return ret;
#endif
}

timespec clock_realtime() {
#ifdef __MACH__
    struct timeval tv;
    int res = gettimeofday(&tv, NULL);
    guarantee_err(res == 0, "gettimeofday failed");

    timespec ret;
    ret.tv_sec = tv.tv_sec;
    ret.tv_nsec = THOUSAND * tv.tv_usec;
    return ret;
#else
    timespec ret;
    int res = clock_gettime(CLOCK_REALTIME, &ret);
    guarantee_err(res == 0, "clock_gettime(CLOCK_REALTIME) failed");
    return ret;
#endif
}

void add_to_timespec(timespec *ts, int32_t nanoseconds) {
    guarantee(ts->tv_nsec >= 0 && ts->tv_nsec < BILLION);
    int64_t new_tv_nsec = ts->tv_nsec + nanoseconds;
    if (new_tv_nsec >= 0 || new_tv_nsec % BILLION == 0) {
        ts->tv_sec += new_tv_nsec / BILLION;
        ts->tv_nsec = new_tv_nsec % BILLION;
    } else {
        ts->tv_sec += new_tv_nsec / BILLION - 1;
        ts->tv_nsec = BILLION + new_tv_nsec % BILLION;
    }
    guarantee(ts->tv_nsec >= 0 && ts->tv_nsec < BILLION);
}

timespec subtract_timespecs(const timespec &t1, const timespec &t2) {
    guarantee(t1.tv_nsec >= 0 && t1.tv_nsec < BILLION);
    guarantee(t2.tv_nsec >= 0 && t2.tv_nsec < BILLION);
    timespec res;
    res.tv_sec = t1.tv_sec - t2.tv_sec;
    if (t2.tv_nsec > t1.tv_nsec) {
        --res.tv_sec;
        res.tv_nsec = t1.tv_nsec + BILLION - t2.tv_nsec;
    } else {
        res.tv_nsec = t1.tv_nsec - t2.tv_nsec;
    }
    guarantee(res.tv_nsec >= 0 && res.tv_nsec < BILLION);
    return res;
}

bool operator<(const struct timespec &t1, const struct timespec &t2) {
    guarantee(t1.tv_nsec >= 0 && t1.tv_nsec < BILLION);
    guarantee(t2.tv_nsec >= 0 && t2.tv_nsec < BILLION);
    return t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_nsec < t2.tv_nsec);
}

bool operator>(const struct timespec &t1, const struct timespec &t2) {
    return t2 < t1;
}

bool operator<=(const struct timespec &t1, const struct timespec &t2) {
    guarantee(t1.tv_nsec >= 0 && t1.tv_nsec < BILLION);
    guarantee(t2.tv_nsec >= 0 && t2.tv_nsec < BILLION);
    return t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_nsec <= t2.tv_nsec);
}

bool operator>=(const struct timespec &t1, const struct timespec &t2) {
    return t2 <= t1;
}

ticks_t get_ticks() {
    timespec tv = clock_monotonic();
    return secs_to_ticks(tv.tv_sec) + tv.tv_nsec;
}

time_t get_secs() {
    timespec tv = clock_realtime();
    return tv.tv_sec;
}

double ticks_to_secs(ticks_t ticks) {
    return ticks / static_cast<double>(BILLION);
}

