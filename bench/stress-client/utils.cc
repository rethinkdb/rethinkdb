// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.hpp"

#if __MACH__
#include <mach/mach_time.h>
#endif

#define THOUSAND 1000LL
#define MILLION (THOUSAND * THOUSAND)
#define BILLION (THOUSAND * MILLION)

/* Timing related functions */
typedef unsigned long long ticks_t;
ticks_t secs_to_ticks(float secs) {
    return (unsigned long long)secs * 1000000000L;
}

// TODO(OSX) copy/pasted from src/utils.cc.
#if __MACH__
mach_timebase_info_data_t mach_time_info;
#endif  // __MACH__

timespec clock_monotonic() {
#if __MACH__
    if (mach_time_info.denom == 0) {
        // TODO(OSX) This is kind of a hack considering multithreading.
        mach_timebase_info(&mach_time_info);
    }
    const uint64_t t = mach_absolute_time();
    uint64_t nanosecs = t * mach_time_info.numer / mach_time_info.denom;
    timespec ret;
    ret.tv_sec = nanosecs / BILLION;
    ret.tv_nsec = nanosecs % BILLION;
    return ret;
#else
    timespec ret;
    int res = clock_gettime(CLOCK_MONOTONIC, &ret);
    guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTIC, ...) failed");
    return ret;
#endif
}

ticks_t get_ticks() {
    timespec tv = clock_monotonic();
    return secs_to_ticks(tv.tv_sec) + tv.tv_nsec;
}

float ticks_to_secs(ticks_t ticks) {
    return ticks / 1000000000.0f;
}

float ticks_to_ms(ticks_t ticks) {
    return ticks / 1000000.0f;
}

float ticks_to_us(ticks_t ticks) {
    return ticks / 1000.0f;
}

void sleep_ticks(ticks_t ticks) {
    int secs = ticks / secs_to_ticks(1);
    if (secs) {
        sleep(secs);
        ticks -= secs_to_ticks(secs);
    }
    if (ticks) {
        usleep(ticks_to_us(ticks));
    }
}

int count_decimal_digits(int n) {
    if (n < 0) {
        fprintf(stderr, "Didn't expect a negative number in count_decimal_digits().\n");
        exit(-1);
    }
    int digits = 1;
    while (n >= 10) {
        n /= 10;
        digits++;
    }
    return digits;
}
