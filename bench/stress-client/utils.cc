#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "utils.hpp"

/* Timing related functions */
typedef unsigned long long ticks_t;
ticks_t secs_to_ticks(float secs) {
    return (unsigned long long)secs * 1000000000L;
}

ticks_t get_ticks() {
    timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return secs_to_ticks(tv.tv_sec) + tv.tv_nsec;
}

long get_ticks_res() {
    timespec tv;
    clock_getres(CLOCK_MONOTONIC, &tv);
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
        usleep(ticks_to_ms(ticks));
    }
}
