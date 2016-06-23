#include "time.hpp"

#include <inttypes.h>

#ifndef _MSC_VER
#include <sys/time.h>
#endif

#ifdef __MACH__
#include <mach/mach_time.h>
#include "thread_local.hpp"
#include "utils.hpp"
#endif

#include "config/args.hpp"
#include "errors.hpp"

#ifdef _MSC_VER

microtime_t current_microtime() {
    FILETIME time;
#if _WIN32_WINNT >= 0x602 // Windows 8
    GetSystemTimePreciseAsFileTime(&time);
#else
    GetSystemTimeAsFileTime(&time);
#endif
    ULARGE_INTEGER nanos100;
    nanos100.LowPart = time.dwLowDateTime;
    nanos100.HighPart = time.dwHighDateTime;
    // Convert to Unix epoch
    nanos100.QuadPart -= 116444736000000000ULL;
    return nanos100.QuadPart / 10;
}

#else

microtime_t current_microtime() {
    // This could be done more efficiently, surely.
    struct timeval t;
    DEBUG_VAR int res = gettimeofday(&t, nullptr);
    rassert(0 == res);
    return uint64_t(t.tv_sec) * MILLION + t.tv_usec;
}

#endif

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
#elif defined(_WIN32)
    timespec ret;
    static THREAD_LOCAL LARGE_INTEGER frequency_hz = {0};
    if (frequency_hz.QuadPart == 0) {
        BOOL res = QueryPerformanceFrequency(&frequency_hz);
        guarantee_winerr(res, "QueryPerformanceFrequency failed");
    }
    LARGE_INTEGER counter;
    BOOL res = QueryPerformanceCounter(&counter);
    guarantee_winerr(res, "QueryPerformanceCounter failed");
    ret.tv_sec = counter.QuadPart / frequency_hz.QuadPart;
    ret.tv_nsec = (counter.QuadPart - ret.tv_sec * frequency_hz.QuadPart) * BILLION / frequency_hz.QuadPart;
    return ret;
#else
    timespec ret;
    int res = clock_gettime(CLOCK_MONOTONIC, &ret);
    guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTIC, ...) failed");
    return ret;
#endif
}

timespec clock_realtime() {
#ifdef __MACH__
    struct timeval tv;
    int res = gettimeofday(&tv, nullptr);
    guarantee_err(res == 0, "gettimeofday failed");

    timespec ret;
    ret.tv_sec = tv.tv_sec;
    ret.tv_nsec = THOUSAND * tv.tv_usec;
    return ret;
#elif defined(_MSC_VER)
    FILETIME time;
#if _WIN32_WINNT >= 0x602 // Windows 8
    GetSystemTimePreciseAsFileTime(&time);
#else
    GetSystemTimeAsFileTime(&time);
#endif
    ULARGE_INTEGER nanos100;
    nanos100.LowPart = time.dwLowDateTime;
    nanos100.HighPart = time.dwHighDateTime;
    // Convert to Unix epoch
    nanos100.QuadPart -= 116444736000000000ULL;
    timespec ret;
    ret.tv_sec = nanos100.QuadPart / (MILLION * 10);
    ret.tv_nsec = (nanos100.QuadPart % (MILLION * 10)) * 100;
    return ret;
#else
    timespec ret;
    int res = clock_gettime(CLOCK_REALTIME, &ret);
    guarantee_err(res == 0, "clock_gettime(CLOCK_REALTIME) failed");
    return ret;
#endif
}

void add_to_timespec(timespec *ts, int32_t nanoseconds) {
    guarantee(ts->tv_nsec >= 0);
    guarantee(ts->tv_nsec < BILLION);
    int64_t new_tv_nsec = ts->tv_nsec + nanoseconds;
    if (new_tv_nsec >= 0 || new_tv_nsec % BILLION == 0) {
        ts->tv_sec += new_tv_nsec / BILLION;
        ts->tv_nsec = new_tv_nsec % BILLION;
    } else {
        ts->tv_sec += new_tv_nsec / BILLION - 1;
        ts->tv_nsec = BILLION + new_tv_nsec % BILLION;
    }
    guarantee(ts->tv_nsec >= 0);
    guarantee(ts->tv_nsec < BILLION);
}

timespec subtract_timespecs(const timespec &t1, const timespec &t2) {
    guarantee(t1.tv_nsec >= 0);
    guarantee(t1.tv_nsec < BILLION);
    guarantee(t2.tv_nsec >= 0);
    guarantee(t2.tv_nsec < BILLION);
    timespec res;
    res.tv_sec = t1.tv_sec - t2.tv_sec;
    if (t2.tv_nsec > t1.tv_nsec) {
        --res.tv_sec;
        res.tv_nsec = t1.tv_nsec + BILLION - t2.tv_nsec;
    } else {
        res.tv_nsec = t1.tv_nsec - t2.tv_nsec;
    }
    guarantee(res.tv_nsec >= 0);
    guarantee(res.tv_nsec < BILLION);
    return res;
}

bool operator<(const struct timespec &t1, const struct timespec &t2) {
    guarantee(t1.tv_nsec >= 0);
    guarantee(t1.tv_nsec < BILLION);
    guarantee(t2.tv_nsec >= 0);
    guarantee(t2.tv_nsec < BILLION);
    return t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_nsec < t2.tv_nsec);
}

bool operator>(const struct timespec &t1, const struct timespec &t2) {
    return t2 < t1;
}

bool operator<=(const struct timespec &t1, const struct timespec &t2) {
    guarantee(t1.tv_nsec >= 0);
    guarantee(t1.tv_nsec < BILLION);
    guarantee(t2.tv_nsec >= 0);
    guarantee(t2.tv_nsec < BILLION);
    return t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_nsec <= t2.tv_nsec);
}

bool operator>=(const struct timespec &t1, const struct timespec &t2) {
    return t2 <= t1;
}

ticks_t get_ticks() {
    timespec tv = clock_monotonic();
    ticks_t ticks = secs_to_ticks(tv.tv_sec) + tv.tv_nsec;
    return ticks;
}

time_t get_secs() {
    timespec tv = clock_realtime();
    return tv.tv_sec;
}

double ticks_to_secs(ticks_t ticks) {
    return ticks / static_cast<double>(BILLION);
}

